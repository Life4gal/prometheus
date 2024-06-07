// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.string:string_pool;

import std;
import gal.prometheus.error;

export namespace gal::prometheus::string
{
	template<typename CharType = char, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
	class StringPool
	{
		class StringBlock
		{
		public:
			constexpr static bool is_null_terminate = IsNullTerminate;

			using view_type = std::basic_string_view<CharType, CharTrait>;
			using value_type = typename view_type::value_type;
			using size_type = typename view_type::size_type;

			constexpr static CharType invalid_char{'\0'};

		private:
			std::unique_ptr<value_type[]> memory_;
			size_type capacity_;
			size_type size_;

		public:
			constexpr explicit StringBlock(const size_type capacity) noexcept
				: memory_{std::make_unique<value_type[]>(capacity)},
				  capacity_{capacity},
				  size_{0} {}

			[[nodiscard]] constexpr static auto length_of(const view_type str) noexcept -> size_type
			{
				if constexpr (is_null_terminate) { return str.length() + 1; }
				else { return str.length(); }
			}

			[[nodiscard]] constexpr auto append(const view_type str) noexcept -> view_type
			{
				if (not this->storable(str))
				{
					GAL_PROMETHEUS_DEBUG_ASSUME(this->storable(str), "There are not enough space for this string.");
					return &invalid_char;
				}

				const auto dest = memory_.get() + size_;
				std::ranges::copy(str, dest);

				if constexpr (is_null_terminate) { dest[str.length()] = 0; }

				size_ += this->length_of(str);
				return {dest, str.length()};
			}

			[[nodiscard]] constexpr auto borrow_raw(const size_type size) noexcept -> value_type*
			{
				if (not this->storable(size))
				{
					GAL_PROMETHEUS_DEBUG_ASSUME(this->storable(size), "There are not enough space for this string.");
					return nullptr;
				}

				const auto dest = memory_.get() + size_;

				if constexpr (is_null_terminate) { dest[size] = 0; }
				size_ += size;

				return dest;
			}

			constexpr auto return_raw(const value_type* raw, const size_type size) noexcept -> void
			{
				auto memory_data = memory_.get();

				GAL_PROMETHEUS_DEBUG_ASSUME(memory_data >= (raw - (size_ - size)), "The given memory does not belong to this block");

				if (memory_data > (raw - (size_ - size)))
				{
					// In this case, new string is inserted after the memory is borrowed, and the newly added string needs to be moved to the front
					auto move_dest = memory_data + (raw - memory_data); // raw;
					GAL_PROMETHEUS_DEBUG_ASSUME(move_dest == raw);
					auto move_source = raw + size;
					auto move_size = size_ - size - (raw - memory_data);

					std::ranges::copy_n(move_source, move_size, move_dest);
				}

				size_ -= size;

				if constexpr (is_null_terminate) { memory_.get()[size_] = 0; }
			}

			[[nodiscard]] constexpr auto storable(const view_type str) const noexcept -> bool { return available_space() >= this->length_of(str); }

			[[nodiscard]] constexpr auto storable(const size_type size) const noexcept -> bool { return available_space() >= size; }

			[[nodiscard]] constexpr auto available_space() const noexcept -> size_type { return capacity_ - size_; }

			[[nodiscard]] constexpr auto more_available_space_than(const StringBlock& other) const noexcept -> bool //
			{
				return available_space() > other.available_space();
			}

			friend constexpr auto swap(StringBlock& lhs, StringBlock& rhs) noexcept -> void
			{
				using std::swap;
				swap(lhs.memory_, rhs.memory_);
				swap(lhs.capacity_, rhs.capacity_);
				swap(lhs.size_, rhs.size_);
			}
		};

		using block_type = StringBlock;
		using pool_type = std::vector<block_type>;

	public:
		using view_type = typename block_type::view_type;
		using value_type = typename block_type::value_type;
		using size_type = typename block_type::size_type;

		constexpr static size_type default_capacity = 8196;

	private:
		pool_type pool_;
		size_type capacity_;

		using block_iterator = typename pool_type::iterator;

	public:
		// fixme: The borrower has many optimizations,
		// such as separating the borrow pool from the actual pool,
		// inserting strings into the borrower pool first,
		// and then inserting it back into the actual pool when the borrower is destructed.
		// The current implementation is to record where all strings are inserted, and then delete them one by one, which involves multiple moves (to move the following strings forward)
		class BlockBorrower
		{
			std::reference_wrapper<StringPool> pool_;
			// pair -> block memory view <=> which block
			std::vector<std::pair<view_type, block_iterator>> borrowed_blocks_;
			bool need_return_ = true;

		public:
			[[nodiscard]] constexpr auto need_return() const noexcept -> bool { return need_return_; }

			constexpr auto break_promise() noexcept -> void { need_return_ = false; }

			constexpr explicit BlockBorrower(StringPool& pool) noexcept
				: pool_{pool} {}

			constexpr BlockBorrower(const BlockBorrower&) = delete;
			constexpr BlockBorrower(BlockBorrower&&) noexcept = default;
			constexpr auto operator=(const BlockBorrower&) -> BlockBorrower& = delete;
			constexpr auto operator=(BlockBorrower&&) -> BlockBorrower& = default;

			constexpr ~BlockBorrower() noexcept
			{
				if (need_return_)
				{
					std::ranges::for_each(
							// back to front, because the last inserted string is saved at the end
							borrowed_blocks_ | std::views::reverse,
							[this](auto& pair) mutable
							{
								auto& pool = pool_.get();

								pool.return_raw_memory(pair.first, pair.second);
								// fixme: maybe a lock is needed here?
								// erase is not performed because the target may still have memory in use.
								// pool.erase(pair.second);
							});
				}
			}

			/**
			 * @brief Add a string to the pool, and then you can freely use the added string.
			 */
			constexpr auto append(const view_type str) -> view_type
			{
				// see also: StringPool::append

				// not only to insert the string, but also to save the inserted position
				auto& pool = pool_.get();

				auto pos = pool.find_or_create_block(str);
				return borrowed_blocks_.emplace_back(pool.append_str_into_block(str, pos), pos).first;
			}

			/**
			 * @brief Borrow a block of memory to the pool, users can directly write strings in this memory area without worrying about its invalidation.
			 */
			[[nodiscard]] constexpr auto borrow_raw(const size_type size) -> value_type*
			{
				// see also: StringPool::borrow_raw

				// not only to insert the string, but also to save the inserted position
				auto& pool = pool_.get();

				auto pos = pool.find_or_create_block(size);
				return borrowed_blocks_.emplace_back({pool.borrow_raw_memory(size, pos), size}, pos).first.data();
			}
		};

	private:
		[[nodiscard]] constexpr auto append_str_into_block(const view_type str, block_iterator pos) -> view_type
		{
			const auto ret = pos->append(str);

			this->shake_it(pos);
			return ret;
		}

		[[nodiscard]] constexpr auto borrow_raw_memory(const size_type size, block_iterator pos) -> value_type*
		{
			auto raw = pos->borrow_raw(size);

			this->shake_it(pos);
			return raw;
		}

		constexpr auto return_raw_memory(const value_type* raw, const size_type size, block_iterator pos) -> void
		{
			pos->return_raw(raw, size);

			this->shake_it(pos);
		}

		constexpr auto return_raw_memory(view_type view, block_iterator pos) -> void { this->return_raw_memory(view.data(), view.size(), pos); }

		[[nodiscard]] constexpr auto find_or_create_block(const size_type size) -> block_iterator
		{
			if (const auto block = this->find_storable_block(size); block != pool_.end()) { return block; }
			return this->create_storable_block(size);
		}

		[[nodiscard]] constexpr auto find_or_create_block(const view_type str) -> block_iterator { return this->find_or_create_block(str.size()); }

		[[nodiscard]] constexpr auto find_first_possible_storable_block(const size_type size) noexcept -> block_iterator
		{
			if (pool_.size() > 2 && not std::ranges::prev(pool_.end(), 2)->storable(size)) { return std::ranges::prev(pool_.end()); }
			return pool_.begin();
		}

		[[nodiscard]] constexpr auto find_first_possible_storable_block(const view_type str) noexcept -> block_iterator
		{
			return this->find_first_possible_storable_block(str.size());
		}

		[[nodiscard]] constexpr auto find_storable_block(const size_type size) noexcept -> block_iterator
		{
			return std::ranges::lower_bound(
					this->find_first_possible_storable_block(size),
					pool_.end(),
					true,
					[](bool b, bool) { return b; },
					[size](const auto& block) { return not block.storable(size); });
		}

		[[nodiscard]] constexpr auto find_storable_block(const view_type str) noexcept -> block_iterator
		{
			return this->find_storable_block(str.size());
		}

		[[nodiscard]] constexpr auto create_storable_block(const size_type size) -> block_iterator
		{
			pool_.emplace_back(std::ranges::max(capacity_, size + IsNullTerminate));
			return std::ranges::prev(pool_.end());
		}

		[[nodiscard]] constexpr auto create_storable_block(const view_type str) -> block_iterator { return this->create_storable_block(str.size()); }

		constexpr auto shake_it(block_iterator block) -> void
		{
			if (
				block == pool_.begin() ||
				block->more_available_space_than(*std::ranges::prev(block))) { return; }

			if (const auto it =
						std::ranges::upper_bound(
								pool_.begin(),
								block,
								block->available_space(),
								std::ranges::less{},
								[](const auto& b) { return b.available_space(); });
				it != block) { std::ranges::rotate(it, block, std::ranges::next(block)); }
		}

	public:
		constexpr explicit StringPool(size_type capacity = default_capacity) noexcept(std::is_nothrow_default_constructible_v<pool_type>)
			: capacity_(capacity) {}

		template<std::same_as<StringPool>... Pools>
		constexpr explicit StringPool(Pools&&... pools) { this->append(std::forward<Pools>(pools)...); }

		template<std::same_as<StringPool>... Pools>
		constexpr auto takeover(Pools&&... pools) -> void
		{
			pool_.reserve(pool_.size() + (pools.pool_.size() + ...));

			block_iterator iterator;
			( //
				( //
					(
						iterator = pool_.insert(pool_.end(), std::make_move_iterator(pools.pool_.begin()), std::make_move_iterator(pools.pool_.end()))
					),
					pools.pool_.clear(),
					std::ranges::inplace_merge(
							pool_.begin(),
							iterator,
							pool_.end(),
							[](const auto& a, const auto& b) { return not a.more_available_space_than(b); })
				),
				...);
		}

		template<std::same_as<StringPool>... Pools>
		constexpr auto copy(const Pools&... pools) -> void
		{
			pool_.reserve(pool_.size() + (pools.pool_.size() + ...));

			block_iterator iterator;
			( //
				( //
					(
						iterator = pool_.insert(pool_.end(), pools.pool_.begin(), pools.pool_.end())
					),
					std::ranges::inplace_merge(
							pool_.begin(),
							iterator,
							pool_.end(),
							[](const auto& a, const auto& b) { return not a.more_available_space_than(b); })
				),
				...);
		}

		/**
		 * @brief Add a string to the pool, and then you can freely use the added string.
		 */
		constexpr auto append(const view_type str) -> view_type { return this->append_str_into_block(str, this->find_or_create_block(str)); }

		/**
		 * @brief Borrow a block of memory to the pool, users can directly write strings in this memory area without worrying about its invalidation.
		 */
		[[nodiscard]] constexpr auto borrow_raw(const size_type size = default_capacity) -> value_type*
		{
			return this->borrow_raw_memory(size, this->find_or_create_block(size));
		}

		/**
		 * @brief User needs to temporarily use some memory area to store the string and return it later
		 */
		[[nodiscard]] constexpr auto borrow_block() noexcept -> BlockBorrower { return BlockBorrower{*this}; }

		[[nodiscard]] constexpr auto size() const noexcept -> size_type { return pool_.size(); }

		[[nodiscard]] constexpr auto capacity() const noexcept -> size_type { return capacity_; }

		/**
		 * @note Only affect the block created after modification
		 */
		constexpr auto resize(size_type capacity) noexcept -> void { capacity_ = capacity; }
	};
}
