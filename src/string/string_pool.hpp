// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <ranges>
#include <type_traits>

#include <prometheus/macro.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::string
{
	namespace string_pool_detail
	{
		template<typename CharType = char, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
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
				: memory_{std::make_unique_for_overwrite<value_type[]>(capacity)},
				  capacity_{capacity},
				  size_{0} {}

			constexpr StringBlock(StringBlock&&) noexcept = default;
			constexpr auto operator=(StringBlock&&) noexcept -> StringBlock& = default;

			// Allow blocks to copy constructs, and furthermore allow pools to copy constructs
			constexpr StringBlock(const StringBlock& other) noexcept
				: memory_{std::make_unique_for_overwrite<value_type[]>(other.capacity_)},
				  capacity_{other.capacity_},
				  size_{other.size_}
			{
				std::ranges::uninitialized_copy_n(
					other.memory_.get(),
					other.size_,
					memory_.get(),
					memory_.get() + other.size_
				);
			}

			// Allow blocks to copy constructs, and furthermore allow pools to copy constructs
			constexpr auto operator=(const StringBlock& other) noexcept -> StringBlock&
			{
				memory_ = std::make_unique_for_overwrite<value_type[]>(other.capacity_);
				capacity_ = other.capacity_;
				size_ = other.size_;

				std::ranges::uninitialized_copy_n(
					other.memory_.get(),
					other.size_,
					memory_.get(),
					memory_.get() + other.size_
				);

				return *this;
			}

			constexpr ~StringBlock() noexcept = default;

			[[nodiscard]] constexpr static auto length_of(const view_type string) noexcept -> size_type
			{
				if constexpr (is_null_terminate)
				{
					return string.length() + 1;
				}
				else
				{
					return string.length();
				}
			}

			[[nodiscard]] constexpr auto append(const view_type string) noexcept -> view_type
			{
				if (not this->storable(string))
				{
					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(this->storable(string), "There are not enough space for this string.");
					return &invalid_char;
				}

				const auto dest = memory_.get() + size_;
				std::ranges::copy(string, dest);

				if constexpr (is_null_terminate)
				{
					dest[string.length()] = 0;
				}

				size_ += StringBlock::length_of(string);
				return {dest, string.length()};
			}

			[[nodiscard]] constexpr auto storable(const view_type str) const noexcept -> bool
			{
				return available_space() >= StringBlock::length_of(str);
			}

			[[nodiscard]] constexpr auto storable(const size_type size) const noexcept -> bool
			{
				return available_space() >= size;
			}

			[[nodiscard]] constexpr auto available_space() const noexcept -> size_type
			{
				return capacity_ - size_;
			}

			[[nodiscard]] constexpr auto more_available_space_than(const StringBlock& other) const noexcept -> bool
			{
				// preserving equivalent elements original order
				return available_space() >= other.available_space();
			}

			friend constexpr auto swap(StringBlock& lhs, StringBlock& rhs) noexcept -> void
			{
				using std::swap;
				swap(lhs.memory_, rhs.memory_);
				swap(lhs.capacity_, rhs.capacity_);
				swap(lhs.size_, rhs.size_);
			}
		};
	}


	template<typename CharType = char, bool IsNullTerminate = true, typename CharTrait = std::char_traits<CharType>>
	class StringPool
	{
		using block_type = string_pool_detail::StringBlock<CharType, IsNullTerminate, CharTrait>;
		using pool_type = std::vector<block_type>;

	public:
		using view_type = typename block_type::view_type;
		using value_type = typename block_type::value_type;
		using size_type = typename block_type::size_type;

		constexpr static size_type default_block_initial_size = 8192;

	private:
		pool_type pool_;
		size_type block_initial_size_;

		using block_iterator = typename pool_type::iterator;

		[[nodiscard]] constexpr auto find_first_possible_storable_block(const size_type length) noexcept -> block_iterator
		{
			if (pool_.size() > 2 && not std::ranges::prev(pool_.end(), 2)->storable(length))
			{
				return std::ranges::prev(pool_.end());
			}
			return pool_.begin();
		}

		[[nodiscard]] constexpr auto find_storable_block(const size_type length) noexcept -> block_iterator
		{
			return std::ranges::partition_point(
				this->find_first_possible_storable_block(length),
				pool_.end(),
				[length](const auto& block)
				{
					return not block.storable(length);
				}
			);
		}

		[[nodiscard]] constexpr auto create_storable_block(const size_type length) -> block_iterator
		{
			pool_.emplace_back(std::ranges::max(block_initial_size_, length + IsNullTerminate));
			return std::ranges::prev(pool_.end());
		}

		[[nodiscard]] constexpr auto find_or_create_block(const size_type length) -> block_iterator
		{
			if (const auto block = this->find_storable_block(length);
				block != pool_.end())
			{
				return block;
			}
			return this->create_storable_block(length);
		}

		constexpr auto shake_it(block_iterator block) -> void
		{
			if (
				block == pool_.begin() ||
				block->more_available_space_than(*std::ranges::prev(block))
			)
			{
				return;
			}

			if (const auto it =
						std::ranges::upper_bound(
							pool_.begin(),
							block,
							block->available_space(),
							std::ranges::less{},
							[](const auto& b)
							{
								return b.available_space();
							});
				it != block)
			{
				std::ranges::rotate(it, block, std::ranges::next(block));
			}
		}

		[[nodiscard]] constexpr auto append_string_into_block(const view_type string, block_iterator block) -> view_type
		{
			const auto view = block->append(string);

			this->shake_it(block);
			return view;
		}

	public:
		constexpr explicit StringPool(size_type block_initial_size = default_block_initial_size) noexcept(std::is_nothrow_default_constructible_v<pool_type>)
			: block_initial_size_(block_initial_size) {}

		template<typename... Pools>
			requires (std::same_as<StringPool, std::remove_cvref_t<Pools>> and ...)
		constexpr explicit StringPool(Pools&&... pools)
		{
			this->join(std::forward<Pools>(pools)...);
		}

		template<typename... Pools>
			requires (std::same_as<StringPool, std::remove_cvref_t<Pools>> and ...)
		constexpr auto join(Pools&&... pools) -> void
		{
			pool_.reserve(pool_.size() + (pools.pool_.size() + ...));

			const auto f = [this]<typename Pool>(Pool&& pool) noexcept -> void
			{
				// silence warning
				auto&& p = std::forward<Pool>(pool);

				const auto size = pool_.size();
				if constexpr (std::is_rvalue_reference_v<Pool&&>)
				{
					pool_.insert(pool_.end(), std::make_move_iterator(p.pool_.begin()), std::make_move_iterator(p.pool_.end()));
					p.pool_.clear();
				}
				else
				{
					pool_.insert_range(pool_.end(), p.pool_);
				}

				std::ranges::inplace_merge(
					pool_,
					std::ranges::next(pool_.begin(), size),
					[](const auto& a, const auto& b)
					{
						return not a.more_available_space_than(b);
					});
			};
			(f.operator()(std::forward<Pools>(pools)), ...);
		}

		/**
		 * @brief Add a string to the pool, and then you can freely use the added string.
		 */
		[[nodiscard]] constexpr auto add(const view_type string) -> view_type
		{
			return this->append_string_into_block(string, this->find_or_create_block(string.size()));
		}

		[[nodiscard]] constexpr auto size() const noexcept -> size_type
		{
			return pool_.size();
		}

		[[nodiscard]] constexpr auto block_initial_size() const noexcept -> size_type
		{
			return block_initial_size_;
		}

		/**
		 * @note Only affect the block created after modification
		 */
		constexpr auto reset_block_initial_size(size_type capacity) noexcept -> void
		{
			block_initial_size_ = capacity;
		}
	};
}
