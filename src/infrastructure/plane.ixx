// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:plane;

import std;
import :runtime_error.terminate_message;
import :concepts;

// https://en.cppreference.com/w/cpp/language/operator_member_access
#if __has_cpp_attribute(__cpp_multidimensional_subscript)
	#define PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT 1
#else
	#define PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT 0
#endif

// https://cplusplus.github.io/LWG/issue3320
// https://wg21.link/p2278r4
// https://en.cppreference.com/w/cpp/iterator/basic_const_iterator
#if GAL_PROMETHEUS_COMPILER_MSVC
	#if __has_cpp_attribute(__cpp_lib_concepts) and _HAS_CXX23
		#define PLANE_PHANTOM_CONST_ITERATOR 1
	#else
		#define PLANE_PHANTOM_CONST_ITERATOR 0
	#endif
#else
	#if __has_cpp_attribute(__cpp_lib_ranges_as_const)
		#define PLANE_PHANTOM_CONST_ITERATOR 1
	#else
		#define PLANE_PHANTOM_CONST_ITERATOR 0
	#endif
#endif

export namespace gal::prometheus::infrastructure
{
	// fixme: default allocator?
	template<typename T>
	using plane_default_allocator = std::allocator<T>;

	template<typename T, concepts::allocator_t Allocator = plane_default_allocator<T>>
	class Plane;
	template<typename T>
	class PlaneView;

	template<typename PlaneType>
	class [[nodiscard]] PlaneRowView : public std::ranges::view_interface<PlaneRowView<PlaneType>>
	{
	public:
		constexpr static auto is_const = std::is_const_v<PlaneType>;

		using plane_type = PlaneType;

		using value_type = typename plane_type::value_type;
		using size_type = typename plane_type::size_type;
		using difference_type = typename plane_type::difference_type;

		using row_type = typename plane_type::row_type;
		using const_row_type = typename plane_type::const_row_type;

	private:
		class [[nodiscard]] Sentinel
		{
			template<bool>
			friend class Iterator;

		public:
			using size_type = size_type;

		private:
			size_type total_row_;

		public:
			constexpr explicit Sentinel(const size_type total_row) noexcept
				: total_row_{total_row} {}
		};

		template<bool IsConst>
		class [[nodiscard]] Iterator
		{
		public:
			using value_type = std::conditional_t<IsConst, const_row_type, row_type>;
			using size_type = size_type;
			using difference_type = difference_type;
			using iterator_concept = std::bidirectional_iterator_tag;
			using iterator_category = std::bidirectional_iterator_tag;

		private:
			std::reference_wrapper<plane_type> plane_;
			size_type                          current_row_;

		public:
			constexpr Iterator(std::reference_wrapper<plane_type> plane, const size_type current_row) noexcept
				: plane_{plane},
				current_row_{current_row} { }

			constexpr auto operator++() noexcept -> Iterator&
			{
				++current_row_;
				return *this;
			}

			constexpr auto operator++(int) noexcept -> Iterator
			{
				Iterator tmp{*this};
				++current_row_;
				return tmp;
			}

			constexpr auto operator--() noexcept -> Iterator&
			{
				--current_row_;
				return *this;
			}

			constexpr auto operator--(int) noexcept -> Iterator
			{
				Iterator tmp{*this};
				--current_row_;
				return tmp;
			}

			[[nodiscard]] constexpr auto operator*() const noexcept -> decltype(auto) { return plane_[current_row_]; }

			[[nodiscard]] constexpr auto operator->() const noexcept -> decltype(auto) { return &plane_[current_row_]; }

			[[nodiscard]] constexpr auto operator==(const Sentinel& sentinel) const noexcept -> bool { return current_row_ == sentinel.total_row_; }

			// ReSharper disable once IdentifierTypo
			[[nodiscard]] friend constexpr auto iter_move(const Iterator& it) noexcept -> decltype(auto) requires(not IsConst) { return std::move(*it); }

			// ReSharper disable once IdentifierTypo
			friend constexpr auto iter_swap(const Iterator& lhs, const Iterator& rhs) noexcept -> void requires(not IsConst)
			{
				using std::swap;

				swap(*lhs, *rhs);
			}
		};

		std::reference_wrapper<plane_type> plane_;

	public:
		constexpr explicit PlaneRowView(std::reference_wrapper<plane_type> plane) noexcept
			: plane_{plane} {}

		[[nodiscard]] constexpr auto begin() noexcept -> Iterator<is_const> { return Iterator<is_const>{plane_, size_type{0}}; }

		[[nodiscard]] constexpr auto end() noexcept -> Sentinel { return Sentinel{plane_.height()}; }

		[[nodiscard]] constexpr auto begin() const noexcept -> Iterator<true> { return Iterator<true>{plane_, size_type{0}}; }

		[[nodiscard]] constexpr auto end() const noexcept -> Sentinel { return Sentinel{plane_.height()}; }
	};

	template<typename T, typename Allocator>
	// ReSharper disable once CppInconsistentNaming
	PlaneRowView(Plane<T, Allocator>&) -> PlaneRowView<Plane<T, Allocator>>;

	template<typename T, typename Allocator>
	// ReSharper disable once CppInconsistentNaming
	PlaneRowView(const Plane<T, Allocator>&) -> PlaneRowView<const Plane<T, Allocator>>;

	template<typename T>
	// ReSharper disable once CppInconsistentNaming
	PlaneRowView(PlaneView<T>&) -> PlaneRowView<PlaneView<T>>;

	template<typename T>
	// ReSharper disable once CppInconsistentNaming
	PlaneRowView(const PlaneView<T>&) -> PlaneRowView<const PlaneView<T>>;

	template<typename T>
	class [[nodiscard]] PlaneView
	{
	public:
		using phantom = std::span<T>;

		using value_type = typename phantom::value_type;
		using size_type = typename phantom::size_type;
		using difference_type = typename phantom::difference_type;
		using pointer = typename phantom::pointer;
		using const_pointer = typename phantom::const_pointer;
		using reference = typename phantom::reference;
		using const_reference = typename phantom::const_reference;
		using iterator = typename phantom::iterator;
		using reverse_iterator = typename phantom::reverse_iterator;
		#if PLANE_PHANTOM_CONST_ITERATOR
		using const_iterator = typename phantom::const_iterator;
		using const_reverse_iterator = typename phantom::const_reverse_iterator;
		#else
		using const_iterator = typename phantom::iterator;
		using const_reverse_iterator = typename phantom::reverse_iterator;
		#endif

		using row_type = std::span<value_type>;
		using const_row_type = std::span<const value_type>;

	private:
		pointer   data_;
		size_type width_;
		size_type height_;
		size_type stride_;

		[[nodiscard]] constexpr auto as_phantom() const noexcept -> phantom { return phantom{data_, size()}; }

	public:
		constexpr PlaneView() noexcept
			: data_{nullptr},
			width_{0},
			height_{0},
			stride_{0} {}

		constexpr PlaneView(
				pointer         data,
				const size_type width,
				const size_type height,
				const size_type stride)
			noexcept
			: data_{data},
			width_{width},
			height_{height},
			stride_{stride} {}

		constexpr PlaneView(
				pointer         data,
				const size_type width,
				const size_type height
				) noexcept
			: PlaneView{data, width, height, width} {}

		template<std::same_as<std::remove_const_t<value_type>> U, typename Allocator>
		constexpr explicit(false) PlaneView(const Plane<U, Allocator>& plane) noexcept
			: PlaneView{plane.data(), plane.width(), plane.height()} {}

		template<std::same_as<std::remove_const_t<value_type>> U, typename Allocator>
		constexpr explicit(false) PlaneView(Plane<U, Allocator>& plane) noexcept
			: PlaneView{plane.data(), plane.width(), plane.height()} { }

		template<std::same_as<std::remove_const_t<value_type>> U, typename Allocator>
		constexpr explicit(false) PlaneView(Plane<U, Allocator>&& plane) noexcept = delete;

		constexpr explicit(false) operator PlaneView<std::add_const_t<value_type>>() const noexcept
			requires(not std::is_const_v<value_type>) { return {data_, width_, height_, stride_}; }

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return width_ == 0 and height_ == 0; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return not empty(); }

		[[nodiscard]] constexpr auto width() const noexcept -> size_type { return width_; }

		[[nodiscard]] constexpr auto height() const noexcept -> size_type { return height_; }

		[[nodiscard]] constexpr auto stride() const noexcept -> size_type { return stride_; }

		[[nodiscard]] constexpr auto size() const noexcept -> size_type { return width_ * height_; }

		[[nodiscard]] constexpr auto data() noexcept -> pointer { return data_; }

		[[nodiscard]] constexpr auto data() const noexcept -> const_pointer { return data_; }

		[[nodiscard]] constexpr auto begin() noexcept -> iterator { return as_phantom().begin(); }

		[[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return as_phantom().begin(); }

		[[nodiscard]] constexpr auto end() noexcept -> iterator { return as_phantom().end(); }

		[[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return as_phantom().end(); }

		[[nodiscard]] constexpr auto operator[](const size_type y) noexcept -> row_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return row_type{data_ + y * stride_, width_};
		}

		[[nodiscard]] constexpr auto operator[](const size_type y) const noexcept -> const_row_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return row_type{data_ + y * stride_, width_};
		}

		#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			[[nodiscard]] constexpr auto operator[](const size_type x, const size_type y) noexcept -> reference
		#else
		[[nodiscard]] constexpr auto operator()(const size_type x,
												const size_type y) noexcept -> reference
			#endif
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(x < width_);
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return data_[y * stride_ + x];
		}

		#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			[[nodiscard]] constexpr auto operator[](const size_type x, const size_type y) const noexcept -> const_reference
		#else
		[[nodiscard]] constexpr auto operator()(const size_type x,
												const size_type y) const noexcept -> const_reference
			#endif
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(x < width_);
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return data_[y * stride_ + x];
		}

		[[nodiscard]] constexpr auto rows() noexcept -> auto { return PlaneRowView{*this}; }

		[[nodiscard]] constexpr auto rows() const noexcept -> auto { return PlaneRowView{*this}; }

		[[nodiscard]] constexpr auto sub_view(const size_type begin_x, const size_type begin_y, const size_type new_width, const size_type new_height) noexcept -> PlaneView<value_type>
		{
			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
				auto* new_data = &this->operator[](begin_x, begin_y);
			#else
			auto* new_data = &this->operator()(begin_x, begin_y);
			#endif

			return PlaneView<value_type>{
					new_data,
					new_width,
					new_height,
					stride_};
		}

		[[nodiscard]] constexpr auto sub_view(const size_type begin_x, const size_type begin_y, const size_type new_width, const size_type new_height) const noexcept -> PlaneView<const value_type>
		{
			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
				const auto* new_data = &this->operator[](begin_x, begin_y);
			#else
			const auto* new_data = &this->operator()(begin_x, begin_y);
			#endif

			return PlaneView<const value_type>{
					new_data,
					new_width,
					new_height,
					stride_};
		}

		friend constexpr auto copy(PlaneView source, PlaneView<std::remove_const_t<value_type>> dest) noexcept -> void// NOLINT
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(source.width() == dest.width(), "Width mismatch!");
			GAL_PROMETHEUS_DEBUG_ASSUME(source.height() == dest.height(), "Height mismatch!");

			if (source.stride() == source.width() and source.stride() == dest.stride()) { std::ranges::copy(source, dest.begin()); }
			else { for (auto& [source_row, dest_row]: std::views::zip(source.rows(), dest.rows())) { std::ranges::copy(source_row, dest_row.begin()); } }
		}

		friend constexpr auto fill(PlaneView dest, const value_type value = value_type{}) noexcept
		{
			if (dest.stride() == dest.width()) { std::ranges::fill(dest, value); }
			else { for (auto& row: dest.rows()) { std::ranges::fill(row, value); } }
		}
	};

	template<typename T, typename S>
		requires std::is_convertible_v<S, typename PlaneView<T>::size_type>
	// ReSharper disable once CppInconsistentNaming
	PlaneView(
			T*                      data,
			S                       width,
			std::type_identity_t<S> height) -> PlaneView<T>;

	template<typename T, typename S>
		requires std::is_convertible_v<S, typename PlaneView<T>::size_type>
	// ReSharper disable once CppInconsistentNaming
	PlaneView(
			T*                      data,
			S                       width,
			std::type_identity_t<S> height,
			std::type_identity_t<S> stride) -> PlaneView<T>;

	template<typename T, typename Allocator>
	// ReSharper disable once CppInconsistentNaming
	PlaneView(const Plane<T, Allocator>& plane) -> PlaneView<const T>;

	template<typename T, typename Allocator>
	// ReSharper disable once CppInconsistentNaming
	PlaneView(Plane<T, Allocator>& plane) -> PlaneView<T>;

	template<typename T, concepts::allocator_t Allocator>
	class Plane
	{
	public:
		using phantom = typename PlaneView<T>::phantom;

		using allocator_type = Allocator;
		using allocator_traits_type = std::allocator_traits<allocator_type>;

		using value_type = typename phantom::value_type;
		using size_type = typename phantom::size_type;
		using difference_type = typename phantom::difference_type;
		using pointer = typename phantom::pointer;
		using const_pointer = typename phantom::const_pointer;
		using reference = typename phantom::reference;
		using const_reference = typename phantom::const_reference;
		using iterator = typename phantom::iterator;
		using reverse_iterator = typename phantom::reverse_iterator;
		#if PLANE_PHANTOM_CONST_ITERATOR
		using const_iterator = typename phantom::const_iterator;
		using const_reverse_iterator = typename phantom::const_reverse_iterator;
		#else
		using const_iterator = typename phantom::iterator;
		using const_reverse_iterator = typename phantom::reverse_iterator;
		#endif

		using row_type = std::span<value_type>;
		using const_row_type = std::span<const value_type>;

	private:
		pointer   data_;
		size_type width_;
		size_type height_;
		size_type capacity_;

		GAL_PROMETHEUS_NO_UNIQUE_ADDRESS allocator_type allocator_;

		[[nodiscard]] constexpr auto as_phantom() const noexcept -> phantom { return phantom{data_, size()}; }

	public:
		constexpr Plane(const Plane& other) noexcept(false)
			: data_{nullptr},
			width_{other.width()},
			height_{other.height()},
			capacity_{other.size()},
			allocator_{allocator_traits_type::select_on_container_copy_construction(other.allocator_)}
		{
			data_ = allocator_traits_type::allocate(allocator_, capacity_);
			std::ranges::uninitialized_copy(other, *this);
		}

		constexpr auto operator=(const Plane& other) noexcept(false) -> Plane&
		{
			if (this == std::addressof(other)) { return *this; }

			constexpr auto propagate_allocator = allocator_traits_type::propagate_on_container_copy_assignment::value;

			const auto replace_allocator = allocator_ != other.allocator_ and propagate_allocator;

			if (capacity_ >= other.size() and not replace_allocator)
			{
				static_assert(std::is_nothrow_copy_constructible_v<value_type>);

				clear();
				width_  = other.width_;
				height_ = other.height_;
				std::ranges::uninitialized_copy(other, *this);
				return *this;
			}

			auto&      new_allocator = replace_allocator ? const_cast<allocator_type&>(other.allocator_) : allocator_;
			const auto new_capacity  = other.size();

			pointer new_data = nullptr;
			try
			{
				new_data = allocator_traits_type::allocate(new_allocator, new_capacity);
				std::ranges::uninitialized_copy(other.begin(), other.end(), new_data, new_data + new_capacity);
			}
			catch (...)
			{
				allocator_traits_type::deallocate(allocator_, new_data, new_capacity);
				throw;
			}

			clear();
			try { shrink_to_fit(); }
			catch (...)
			{
				std::ranges::destroy_n(new_data, new_capacity);
				allocator_traits_type::deallocate(allocator_, new_data, new_capacity);
				throw;
			}

			data_      = new_data;
			width_     = other.width_;
			height_    = other.height_;
			capacity_  = new_capacity;
			allocator_ = new_allocator;
			return *this;
		}

		constexpr Plane(Plane&& other) noexcept
			: data_{std::exchange(other.data_, nullptr)},
			width_{std::exchange(other.width_, 0)},
			height_{std::exchange(other.height_, 0)},
			capacity_{std::exchange(other.capacity_, 0)},
			allocator_{std::exchange(other.allocator_, allocator_type{})} {}

		constexpr auto operator=(Plane&& other) noexcept(false) -> Plane&
		{
			if (this == std::addressof(other)) { return *this; }

			constexpr auto propagate_allocator = allocator_traits_type::propagate_on_container_copy_assignment::value;

			if (allocator_ == other.allocator_ or not propagate_allocator)
			{
				clear();
				shrink_to_fit();

				data_      = std::exchange(other.data_, nullptr);
				width_     = std::exchange(other.width_, 0);
				height_    = std::exchange(other.height_, 0);
				capacity_  = std::exchange(other.capacity_, 0);
				allocator_ = std::exchange(other.allocator_, {});
				return *this;
			}

			if (capacity_ >= other.size())
			{
				clear();

				width_  = other.width_;
				height_ = other.height_;

				std::ranges::uninitialized_move(other, *this);

				// Clear, but leave the allocation intact, so that it can be reused.
				other.clear();
				return *this;
			}

			const auto new_capacity = other.size();

			pointer new_data = nullptr;
			try
			{
				new_data = allocator_traits_type::allocate(allocator_, new_capacity);
				std::ranges::uninitialized_move(other.begin(), other.end(), new_data, new_data + new_capacity);
			}
			catch (...)
			{
				allocator_traits_type::deallocate(allocator_, new_data, new_capacity);
				throw;
			}

			clear();
			try { shrink_to_fit(); }
			catch (...)
			{
				std::ranges::destroy_n(new_data, new_capacity);
				allocator_traits_type::deallocate(allocator_, new_data, new_capacity);
				throw;
			}

			data_     = new_data;
			width_    = other.width_;
			height_   = other.height_;
			capacity_ = new_capacity;

			// Clear, but leave the allocation intact, so that it can be reused.
			other.clear();
			return *this;
		}

		constexpr ~Plane() noexcept
		{
			// fixme: span range
			// std::ranges::destroy(*this);
			std::ranges::destroy(begin(), end());
			if (data_) { allocator_traits_type::deallocate(allocator_, data_, capacity_); }
		}

		constexpr Plane() noexcept
			: data_{nullptr},
			width_{0},
			height_{0},
			capacity_{0},
			allocator_{} {}

		constexpr Plane(
				const size_type width,
				const size_type height,
				allocator_type  allocator = allocator_type{}) noexcept(false)
			: data_{allocator_traits_type::allocate(allocator, width * height)},
			width_{width},
			height_{height},
			capacity_{width * height},
			allocator_{allocator} { std::ranges::uninitialized_value_construct(*this); }

		template<std::convertible_to<value_type> U>
		constexpr Plane(
				const U*        source,
				const size_type width,
				const size_type height,
				const size_type stride,
				allocator_type  allocator = allocator_type{}) noexcept(false)
			: data_{allocator_traits_type::allocate(allocator, width * height)},
			width_{width},
			height_{height},
			capacity_{width * height},
			allocator_{allocator}
		{
			GAL_PROMETHEUS_DEBUG_NOT_NULL(source);

			if (width_ == stride)
			{
				try { std::ranges::uninitialized_copy(source, source + capacity_, data_, data_ + capacity_); }
				catch (...)
				{
					allocator_traits_type::deallocate(allocator_, data_, capacity_);
					throw;
				}
			}
			else
			{
				auto it_source   = source;
				auto it_dest     = data_;
				auto it_dest_end = data_ + capacity_;

				try
				{
					while (it_dest != it_dest_end)
					{
						std::ranges::uninitialized_copy(it_source, it_source + width, it_dest, it_dest + width);
						it_source += stride;
						it_dest += width;
					}
				}
				catch (...)
				{
					std::ranges::destroy(data_, it_dest);
					allocator_traits_type::deallocate(allocator_, data_, capacity_);
					throw;
				}
			}
		}

		template<std::convertible_to<value_type> U>
		constexpr Plane(
				const U*        source,
				const size_type width,
				const size_type height,
				allocator_type  allocator = allocator_type{}) noexcept(false)
			: Plane{source, width, height, width, allocator} {}

		template<std::convertible_to<value_type> U>
		constexpr explicit Plane(
				const PlaneView<U> other,
				allocator_type     allocator = allocator_type{}) noexcept(false)
			: Plane{other.data(), other.width(), other.height(), other.stride(), allocator} { }

		template<std::convertible_to<value_type> U, typename A>
		constexpr explicit Plane(
				const Plane<U, A>& other,
				allocator_type     allocator = allocator_type{}) noexcept(false)
			: Plane{other.operator PlaneView<const U>(), allocator} { }

		constexpr explicit(false) operator PlaneView<value_type>() const noexcept { return {data_, width_, height_}; }

		constexpr explicit(false) operator PlaneView<const value_type>() const noexcept { return {data_, width_, height_}; }

		constexpr auto operator==(const Plane& other) const noexcept -> bool
		{
			if (width() != other.width() or height() != other.height()) { return false; }

			return std::ranges::equal(other, *this);
		}

		[[nodiscard]] constexpr auto get_allocator() const noexcept -> allocator_type { return allocator_; }

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return width() == 0 and height() == 0; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return not empty(); }

		[[nodiscard]] constexpr auto width() const noexcept -> size_type { return width_; }

		[[nodiscard]] constexpr auto height() const noexcept -> size_type { return height_; }

		[[nodiscard]] constexpr auto size() const noexcept -> size_type { return width_ * height_; }

		[[nodiscard]] constexpr auto capacity() const noexcept -> size_type { return capacity_; }

		[[nodiscard]] constexpr auto data() noexcept -> pointer { return data_; }

		[[nodiscard]] constexpr auto data() const noexcept -> const_pointer { return data_; }

		[[nodiscard]] constexpr auto begin() noexcept -> iterator { return as_phantom().begin(); }

		[[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return as_phantom().begin(); }

		[[nodiscard]] constexpr auto end() noexcept -> iterator { return as_phantom().end(); }

		[[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return as_phantom().end(); }

		[[nodiscard]] constexpr auto operator[](const size_type y) noexcept -> row_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return row_type{data_ + y * width_, width_};
		}

		[[nodiscard]] constexpr auto operator[](const size_type y) const noexcept -> const_row_type
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return const_row_type{data_ + y * width_, width_};
		}

		#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			[[nodiscard]] constexpr auto operator[](const size_type x, const size_type y) noexcept -> reference
		#else
		[[nodiscard]] constexpr auto operator()(const size_type x,
												const size_type y) noexcept -> reference
			#endif
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(x < width_);
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return data_[y * width_ + x];
		}

		#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			[[nodiscard]] constexpr auto operator[](const size_type x, const size_type y) const noexcept -> const_reference
		#else
		[[nodiscard]] constexpr auto operator()(const size_type x,
												const size_type y) const noexcept -> const_reference
			#endif
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(x < width_);
			GAL_PROMETHEUS_DEBUG_ASSUME(y < height_);

			return data_[y * width_ + x];
		}

		[[nodiscard]] constexpr auto rows() noexcept -> auto { return PlaneRowView{*this}; }

		[[nodiscard]] constexpr auto rows() const noexcept -> auto { return PlaneRowView{*this}; }

		[[nodiscard]] constexpr auto sub_view(const size_type begin_x, const size_type begin_y, const size_type new_width, const size_type new_height) noexcept -> PlaneView<const value_type> { return operator PlaneView<value_type>().sub_view(begin_x, begin_y, new_width, new_height); }

		[[nodiscard]] constexpr auto sub_plane(const size_type begin_x, const size_type begin_y, const size_type new_width, const size_type new_height, allocator_type allocator) noexcept(false) -> Plane
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(begin_x + new_width <= width_ and begin_y + new_height <= height_);

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
				const auto* new_data = &this->operator[](begin_x, begin_y);
			#else
			const auto* new_data = &this->operator()(begin_x, begin_y);
			#endif

			return {
					new_data,
					new_width,
					new_height,
					width_,
					allocator};
		}

		[[nodiscard]] constexpr auto sub_plane(const size_type begin_x, const size_type begin_y, const size_type new_width, const size_type new_height) noexcept(false) -> Plane { return this->sub_plane(begin_x, begin_y, new_width, new_height, allocator_); }

		constexpr auto clear() noexcept -> void
		{
			std::ranges::destroy(*this);
			width_  = 0;
			height_ = 0;
		}

		constexpr auto shrink_to_fit() noexcept(false) -> void
		{
			if (empty())
			{
				if (data_)
				{
					allocator_traits_type::deallocate(allocator_, data_, capacity_);
					data_     = nullptr;
					capacity_ = 0;
				}
				return;
			}

			const auto new_capacity = size();

			pointer new_data = nullptr;
			try
			{
				new_data = allocator_traits_type::allocate(allocator_, new_capacity);
				std::ranges::uninitialized_move(begin(), end(), new_data, new_data + new_capacity);
			}
			catch (...)
			{
				allocator_traits_type::deallocate(allocator_, new_data, new_capacity);
				throw;
			}

			std::ranges::destroy(*this);
			const auto old_data     = std::exchange(data_, new_data);
			const auto old_capacity = std::exchange(capacity_, new_capacity);
			allocator_traits_type::deallocate(allocator_, old_data, old_capacity);
		}

		friend constexpr auto fill(Plane& dest, const value_type value = value_type{}) noexcept { std::ranges::fill(dest, value); }
	};

	template<typename T, typename S, typename Allocator = plane_default_allocator<T>>
		requires std::is_convertible_v<S, typename Plane<T, Allocator>::size_type> and std::is_same_v<Allocator, typename Plane<T, Allocator>::allocator_type>
	// ReSharper disable once CppInconsistentNaming
	Plane(
			T*                      data,
			S                       width,
			std::type_identity_t<S> height,
			std::type_identity_t<S> stride,
			Allocator               allocator = Allocator{}
			) -> Plane<T, Allocator>;

	template<typename T, typename S, typename Allocator = plane_default_allocator<T>>
		requires std::is_convertible_v<S, typename Plane<T, Allocator>::size_type> and std::is_same_v<Allocator, typename Plane<T, Allocator>::allocator_type>
	// ReSharper disable once CppInconsistentNaming
	Plane(
			const T*                data,
			S                       width,
			std::type_identity_t<S> height,
			std::type_identity_t<S> stride,
			Allocator               allocator = Allocator{}
			) -> Plane<T, Allocator>;

	template<typename T, typename S, typename Allocator = plane_default_allocator<T>>
		requires std::is_convertible_v<S, typename Plane<T, Allocator>::size_type> and std::is_same_v<Allocator, typename Plane<T, Allocator>::allocator_type>
	// ReSharper disable once CppInconsistentNaming
	Plane(
			T*                      data,
			S                       width,
			std::type_identity_t<S> height,
			Allocator               allocator = Allocator{}
			) -> Plane<T, Allocator>;

	template<typename T, typename S, typename Allocator = plane_default_allocator<T>>
		requires std::is_convertible_v<S, typename Plane<T, Allocator>::size_type> and std::is_same_v<Allocator, typename Plane<T, Allocator>::allocator_type>
	// ReSharper disable once CppInconsistentNaming
	Plane(
			const T*                data,
			S                       width,
			std::type_identity_t<S> height,
			Allocator               allocator = Allocator{}
			) -> Plane<T, Allocator>;

	template<typename T, typename Allocator = plane_default_allocator<T>>
		requires std::is_same_v<Allocator, typename Plane<std::remove_const_t<T>, Allocator>::allocator_type>
	// ReSharper disable once CppInconsistentNaming
	Plane(PlaneView<T> source,
	      Allocator    allocator = Allocator{}
			) -> Plane<std::remove_const_t<T>, Allocator>;
}
