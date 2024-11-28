// meta::dimension
#include <meta/meta.hpp>

using namespace gal::prometheus;

namespace
{
	namespace from
	{
		struct point_float
		{
			float x;
			float y;
		};

		struct point_int : meta::dimension<point_int>
		{
			int x;
			int y;
		};

		static_assert(sizeof(point_float) == sizeof(float) * 2);
		static_assert(sizeof(point_float) == sizeof(point_int));
		static_assert(std::is_trivially_constructible_v<point_float>);
		static_assert(std::is_trivially_constructible_v<point_int>);

		constexpr point_float pf_1{.x = 42.12345f, .y = 1337.12345f};

		constexpr point_int pi_1{.x = 42, .y = 1337};
		constexpr point_int pi_2{.x = 42, .y = 42};

		static_assert(std::ranges::all_of(point_int::from(pf_1) == pi_1, std::identity{}));
		static_assert(std::ranges::all_of(point_int::from(pf_1.x) == pi_2, std::identity{}));
		static_assert(std::ranges::all_of(point_int::from(pi_1.x) == pi_2, std::identity{}));
	}

	namespace to
	{
		struct point_float : meta::dimension<point_float>
		{
			float x;
			float y;
		};

		struct point_int
		{
			int x;
			int y;

			[[nodiscard]] constexpr auto operator==(const point_int& other) const noexcept -> bool
			{
				return x == other.x and y == other.y;
			}
		};

		static_assert(sizeof(point_float) == sizeof(float) * 2);
		static_assert(sizeof(point_float) == sizeof(point_int));
		static_assert(std::is_trivially_constructible_v<point_float>);
		static_assert(std::is_trivially_constructible_v<point_int>);

		constexpr point_float pf_1{.x = 42.12345f, .y = 1337.12345f};
		constexpr point_int pi_1{.x = 42, .y = 1337};

		constexpr auto t1 = pf_1.to();
		static_assert(t1.x == pf_1.x); // NOLINT(clang-diagnostic-float-equal)
		static_assert(t1.y == pf_1.y); // NOLINT(clang-diagnostic-float-equal)

		constexpr auto t2 = pf_1.to<point_int>();
		static_assert(t2.x == pi_1.x);
		static_assert(t2.y == pi_1.y);

		constexpr auto to_int_1 = [](const auto value) noexcept -> int
		{
			return static_cast<int>(value) + 123;
		};

		constexpr auto t3 = pf_1.to<point_int>(to_int_1);
		static_assert(t3.x == pi_1.x + 123);
		static_assert(t3.y == pi_1.y + 123);

		constexpr auto to_int_2 = []<std::size_t Index>(const auto value) noexcept -> int
		{
			return static_cast<int>(value) + 123 * Index;
		};

		constexpr auto t4 = pf_1.to<point_int>(to_int_2);
		static_assert(t4.x == pi_1.x + 123 * 0);
		static_assert(t4.y == pi_1.y + 123 * 1);

		struct to_int_3
		{
			template<std::size_t Index>
			[[nodiscard]] constexpr auto operator()(const auto value) const noexcept -> decltype(auto)
			{
				if constexpr (Index == 0)
				{
					return to_int_1(value);
				}
				else
				{
					return to_int_2.operator()<Index>(value);
				}
			}
		};

		constexpr auto t5 = pf_1.to<point_int>(to_int_3{});
		static_assert(t5.x == pi_1.x + 123);
		static_assert(t5.y == pi_1.y + 123 * 1);
	}

	namespace addition
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator+(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value + other.value};
			}

			[[nodiscard]] constexpr auto operator+(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value + other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator+=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value += other.value;
				return *this;
			}

			constexpr auto operator+=(const int& other) noexcept -> wrapper_x_equal&
			{
				value += other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() + std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() += std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs + rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template add<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t += rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template add_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x + point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value + px2.x.value, .y = px1.y.value + px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value + px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value + px2.y.value}, std::identity{}));
		// point_x + point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value + p.x, .y = px1.y.value + p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value + p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value + p.y}, std::identity{}));
		// point_x + value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value + 13579, .y = px1.y.value + 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value + 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value + 13579}, std::identity{}));
		// point_x_equal += point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value + pxe2.x.value, .y = pxe1.y.value + pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value + pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value + pxe2.y.value}, std::identity{}));
		// point_x_equal += point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value + p.x, .y = pxe1.y.value + p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value + p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value + p.y}, std::identity{}));
		// point_x_equal += value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value + 13579, .y = pxe1.y.value + 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value + 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value + 13579}, std::identity{}));
	}

	namespace subtraction
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator-(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value - other.value};
			}

			[[nodiscard]] constexpr auto operator-(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value - other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator-=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value -= other.value;
				return *this;
			}

			constexpr auto operator-=(const int& other) noexcept -> wrapper_x_equal&
			{
				value -= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() - std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() -= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs - rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template subtract<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t -= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template subtract_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x - point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value - px2.x.value, .y = px1.y.value - px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value - px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value - px2.y.value}, std::identity{}));
		// point_x - point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value - p.x, .y = px1.y.value - p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value - p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value - p.y}, std::identity{}));
		// point_x - value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value - 13579, .y = px1.y.value - 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value - 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value - 13579}, std::identity{}));
		// point_x_equal -= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value - pxe2.x.value, .y = pxe1.y.value - pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value - pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value - pxe2.y.value}, std::identity{}));
		// point_x_equal -= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value - p.x, .y = pxe1.y.value - p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value - p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value - p.y}, std::identity{}));
		// point_x_equal -= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value - 13579, .y = pxe1.y.value - 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value - 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value - 13579}, std::identity{}));
	}

	namespace multiplication
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator*(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value * other.value};
			}

			[[nodiscard]] constexpr auto operator*(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value * other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator*=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value *= other.value;
				return *this;
			}

			constexpr auto operator*=(const int& other) noexcept -> wrapper_x_equal&
			{
				value *= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() * std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() *= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs * rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template multiply<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t *= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template multiply_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x * point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value * px2.x.value, .y = px1.y.value * px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value * px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value * px2.y.value}, std::identity{}));
		// point_x * point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value * p.x, .y = px1.y.value * p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value * p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value * p.y}, std::identity{}));
		// point_x * value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value * 13579, .y = px1.y.value * 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value * 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value * 13579}, std::identity{}));
		// point_x_equal *= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value * pxe2.x.value, .y = pxe1.y.value * pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value * pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value * pxe2.y.value}, std::identity{}));
		// point_x_equal *= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value * p.x, .y = pxe1.y.value * p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value * p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value * p.y}, std::identity{}));
		// point_x_equal *= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value * 13579, .y = pxe1.y.value * 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value * 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value * 13579}, std::identity{}));
	}

	namespace division
	{
		struct wrapper_x
		{
			float value;

			// point_x == point ==> point_x.members == point.members ==> x == float
			[[nodiscard]] constexpr auto operator==(const float other) const noexcept -> bool
			{
				return value == other; // NOLINT(clang-diagnostic-float-equal)
			}

			[[nodiscard]] constexpr auto operator/(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value / other.value};
			}

			[[nodiscard]] constexpr auto operator/(const float& other) const noexcept -> wrapper_x
			{
				return {.value = value / other};
			}
		};

		struct wrapper_x_equal
		{
			float value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == float
			[[nodiscard]] constexpr auto operator==(const float other) const noexcept -> bool
			{
				return value == other; // NOLINT(clang-diagnostic-float-equal)
			}

			constexpr auto operator/=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value /= other.value;
				return *this;
			}

			constexpr auto operator/=(const float& other) noexcept -> wrapper_x_equal&
			{
				value /= other;
				return *this;
			}
		};

		struct point
		{
			float x;
			float y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(float) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() / std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() /= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs / rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template divide<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t /= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template divide_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x / point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value / px2.x.value, .y = px1.y.value / px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value / px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value / px2.y.value}, std::identity{}));
		// point_x / point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value / p.x, .y = px1.y.value / p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value / p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value / p.y}, std::identity{}));
		// point_x / value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value / 13579, .y = px1.y.value / 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value / 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value / 13579}, std::identity{}));
		// point_x_equal /= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value / pxe2.x.value, .y = pxe1.y.value / pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value / pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value / pxe2.y.value}, std::identity{}));
		// point_x_equal /= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value / p.x, .y = pxe1.y.value / p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value / p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value / p.y}, std::identity{}));
		// point_x_equal /= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value / 13579, .y = pxe1.y.value / 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value / 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value / 13579}, std::identity{}));
	}

	namespace modulus
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator%(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value % other.value};
			}

			[[nodiscard]] constexpr auto operator%(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value % other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator%=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value %= other.value;
				return *this;
			}

			constexpr auto operator%=(const int& other) noexcept -> wrapper_x_equal&
			{
				value %= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() % std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() %= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs % rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template mod<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t %= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template mod_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x % point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value % px2.x.value, .y = px1.y.value % px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value % px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value % px2.y.value}, std::identity{}));
		// point_x % point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value % p.x, .y = px1.y.value % p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value % p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value % p.y}, std::identity{}));
		// point_x % value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value % 13579, .y = px1.y.value % 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value % 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value % 13579}, std::identity{}));
		// point_x_equal %= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value % pxe2.x.value, .y = pxe1.y.value % pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value % pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value % pxe2.y.value}, std::identity{}));
		// point_x_equal %= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value % p.x, .y = pxe1.y.value % p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value % p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value % p.y}, std::identity{}));
		// point_x_equal %= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value % 13579, .y = pxe1.y.value % 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value % 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value % 13579}, std::identity{}));
	}

	namespace bit_and
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator&(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value & other.value};
			}

			[[nodiscard]] constexpr auto operator&(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value & other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator&=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value &= other.value;
				return *this;
			}

			constexpr auto operator&=(const int& other) noexcept -> wrapper_x_equal&
			{
				value &= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() & std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() &= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs & rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template bit_and<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t &= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template bit_and_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x & point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value & px2.x.value, .y = px1.y.value & px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value & px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value & px2.y.value}, std::identity{}));
		// point_x & point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value & p.x, .y = px1.y.value & p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value & p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value & p.y}, std::identity{}));
		// point_x & value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value & 13579, .y = px1.y.value & 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value & 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value & 13579}, std::identity{}));
		// point_x_equal &= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value & pxe2.x.value, .y = pxe1.y.value & pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value & pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value & pxe2.y.value}, std::identity{}));
		// point_x_equal &= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value & p.x, .y = pxe1.y.value & p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value & p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value & p.y}, std::identity{}));
		// point_x_equal &= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value & 13579, .y = pxe1.y.value & 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value & 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value & 13579}, std::identity{}));
	}

	namespace bit_or
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator|(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value | other.value};
			}

			[[nodiscard]] constexpr auto operator|(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value | other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator|=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value |= other.value;
				return *this;
			}

			constexpr auto operator|=(const int& other) noexcept -> wrapper_x_equal&
			{
				value |= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() | std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() |= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs | rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template bit_or<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t |= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template bit_or_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x | point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value | px2.x.value, .y = px1.y.value | px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value | px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value | px2.y.value}, std::identity{}));
		// point_x | point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value | p.x, .y = px1.y.value | p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value | p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value | p.y}, std::identity{}));
		// point_x | value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value | 13579, .y = px1.y.value | 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value | 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value | 13579}, std::identity{}));
		// point_x_equal |= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value | pxe2.x.value, .y = pxe1.y.value | pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value | pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value | pxe2.y.value}, std::identity{}));
		// point_x_equal |= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value | p.x, .y = pxe1.y.value | p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value | p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value | p.y}, std::identity{}));
		// point_x_equal |= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value | 13579, .y = pxe1.y.value | 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value | 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value | 13579}, std::identity{}));
	}

	namespace bit_xor
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator^(const wrapper_x& other) const noexcept -> wrapper_x
			{
				return {.value = value ^ other.value};
			}

			[[nodiscard]] constexpr auto operator^(const int& other) const noexcept -> wrapper_x
			{
				return {.value = value ^ other};
			}
		};

		struct wrapper_x_equal
		{
			int value;

			// point_x_equal == point ==> point_x_equal.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			constexpr auto operator^=(const wrapper_x_equal& other) noexcept -> wrapper_x_equal&
			{
				value ^= other.value;
				return *this;
			}

			constexpr auto operator^=(const int& other) noexcept -> wrapper_x_equal&
			{
				value ^= other;
				return *this;
			}
		};

		struct point
		{
			int x;
			int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			wrapper_x x;
			wrapper_x y;
		};

		struct point_x_equal : meta::dimension<point_x_equal>
		{
			wrapper_x_equal x;
			wrapper_x_equal y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(sizeof(point) == sizeof(point_x_equal));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);
		static_assert(std::is_trivially_constructible_v<point_x_equal>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<L&>() = std::declval<const L&>() ^ std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);
		static_assert(not can_x<point_x_equal, point>);

		template<typename L, typename R>
		constexpr auto can_x_equal = requires { std::declval<L&>() ^= std::declval<const R&>(); };
		static_assert(not can_x_equal<point_x, point>);
		static_assert(can_x_equal<point_x_equal, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		constexpr point_x_equal pxe1{.x = {.value = 6789}, .y = {.value = 1234}};
		constexpr point_x_equal pxe2{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs ^ rhs;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs.template bit_xor<static_cast<meta::Dimensions>(Index)>(rhs);
		}

		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t ^= rhs;
			return t;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x_equal(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			auto t = lhs.copy();
			t.template bit_xor_equal<static_cast<meta::Dimensions>(Index)>(rhs);
			return t;
		}

		// point_x ^ point_x
		static_assert(std::ranges::all_of(do_x(px1, px2) == point{.x = px1.x.value ^ px2.x.value, .y = px1.y.value ^ px2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, px2) == point{.x = px1.x.value ^ px2.x.value, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, px2) == point{.x = px1.x.value, .y = px1.y.value ^ px2.y.value}, std::identity{}));
		// point_x ^ point
		static_assert(std::ranges::all_of(do_x(px1, p) == point{.x = px1.x.value ^ p.x, .y = px1.y.value ^ p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, p) == point{.x = px1.x.value ^ p.x, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, p) == point{.x = px1.x.value, .y = px1.y.value ^ p.y}, std::identity{}));
		// point_x ^ value
		static_assert(std::ranges::all_of(do_x(px1, 13579) == point{.x = px1.x.value ^ 13579, .y = px1.y.value ^ 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px1, 13579) == point{.x = px1.x.value ^ 13579, .y = px1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px1, 13579) == point{.x = px1.x.value, .y = px1.y.value ^ 13579}, std::identity{}));
		// point_x_equal ^= point_x_equal
		static_assert(std::ranges::all_of(do_x_equal(pxe1, pxe2) == point{.x = pxe1.x.value ^ pxe2.x.value, .y = pxe1.y.value ^ pxe2.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, pxe2) == point{.x = pxe1.x.value ^ pxe2.x.value, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, pxe2) == point{.x = pxe1.x.value, .y = pxe1.y.value ^ pxe2.y.value}, std::identity{}));
		// point_x_equal ^= point
		static_assert(std::ranges::all_of(do_x_equal(pxe1, p) == point{.x = pxe1.x.value ^ p.x, .y = pxe1.y.value ^ p.y}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, p) == point{.x = pxe1.x.value ^ p.x, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, p) == point{.x = pxe1.x.value, .y = pxe1.y.value ^ p.y}, std::identity{}));
		// point_x_equal ^= value
		static_assert(std::ranges::all_of(do_x_equal(pxe1, 13579) == point{.x = pxe1.x.value ^ 13579, .y = pxe1.y.value ^ 13579}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<0>(pxe1, 13579) == point{.x = pxe1.x.value ^ 13579, .y = pxe1.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x_equal<1>(pxe1, 13579) == point{.x = pxe1.x.value, .y = pxe1.y.value ^ 13579}, std::identity{}));
	}

	namespace bit_flip
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator~() const noexcept -> wrapper_x
			{
				return {.value = ~value};
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename T>
		constexpr auto can_x = requires { std::declval<T&>() = ~std::declval<const T&>(); };
		static_assert(can_x<point_x>);

		constexpr point_x px{.x = {.value = 1234}, .y = {.value = 6789}};

		[[nodiscard]] constexpr auto do_x(const auto& self) noexcept -> auto
		{
			return ~self;
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto do_x(const auto& self) noexcept -> auto
		{
			return self.template bit_flip<static_cast<meta::Dimensions>(Index)>();
		}

		// ~point_x
		static_assert(std::ranges::all_of(do_x(px) == point{.x = ~px.x.value, .y = ~px.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<0>(px) == point{.x = ~px.x.value, .y = px.y.value}, std::identity{}));
		static_assert(std::ranges::all_of(do_x<1>(px) == point{.x = px.x.value, .y = ~px.y.value}, std::identity{}));
	}

	namespace logical_and
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator and(const wrapper_x& other) const noexcept -> bool
			{
				return value and other.value;
			}

			[[nodiscard]] constexpr auto operator and(const int& other) const noexcept -> bool
			{
				return value and other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() and std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs and rhs;
		}

		// point_x and point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x and point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x and value
		static_assert(std::ranges::all_of(do_x(px1, 13579), std::identity{}));
	}

	namespace logical_or
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator||(const wrapper_x& other) const noexcept -> bool
			{
				return value || other.value;
			}

			[[nodiscard]] constexpr auto operator||(const int& other) const noexcept -> bool
			{
				return value || other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() or std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs or rhs;
		}

		// point_x or point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x or point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x or value
		static_assert(std::ranges::all_of(do_x(px1, 13579), std::identity{}));
	}

	namespace logical_not
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator not() const noexcept -> bool
			{
				return not value;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename T>
		constexpr auto can_x = requires { not std::declval<const T&>(); };
		static_assert(can_x<point_x>);

		constexpr point_x px{.x = {.value = 1234}, .y = {.value = 6789}};

		// not point_x
		static_assert(std::ranges::none_of(not px, std::identity{}));
	}

	namespace greater_than
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator>(const wrapper_x& other) const noexcept -> bool
			{
				return value > other.value;
			}

			[[nodiscard]] constexpr auto operator>(const int& other) const noexcept -> bool
			{
				return value > other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() > std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 12340}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs > rhs;
		}

		// point_x > point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x > point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x > value
		static_assert(std::ranges::all_of(do_x(px1, 1357), std::identity{}));
	}

	namespace greater_equal
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator>=(const wrapper_x& other) const noexcept -> bool
			{
				return value >= other.value;
			}

			[[nodiscard]] constexpr auto operator>=(const int& other) const noexcept -> bool
			{
				return value >= other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() >= std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 42, .y = 1337};

		constexpr point_x px1{.x = {.value = 12340}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 1234}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs >= rhs;
		}

		// point_x >= point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x >= point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x >= value
		static_assert(std::ranges::all_of(do_x(px1, 1357), std::identity{}));
	}

	namespace less_than
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator<(const wrapper_x& other) const noexcept -> bool
			{
				return value < other.value;
			}

			[[nodiscard]] constexpr auto operator<(const int& other) const noexcept -> bool
			{
				return value < other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() < std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 4200, .y = 13370};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 12340}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs < rhs;
		}

		// point_x < point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x < point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x < value
		static_assert(std::ranges::all_of(do_x(px1, 13579), std::identity{}));
	}

	namespace less_equal
	{
		struct wrapper_x
		{
			int value;

			// point_x == point ==> point_x.members == point.members ==> x == int
			[[nodiscard]] constexpr auto operator==(const int other) const noexcept -> bool
			{
				return value == other;
			}

			[[nodiscard]] constexpr auto operator<=(const wrapper_x& other) const noexcept -> bool
			{
				return value <= other.value;
			}

			[[nodiscard]] constexpr auto operator<=(const int& other) const noexcept -> bool
			{
				return value <= other;
			}
		};

		struct point
		{
			[[maybe_unused]] int x;
			[[maybe_unused]] int y;
		};

		struct point_x : meta::dimension<point_x>
		{
			[[maybe_unused]] wrapper_x x;
			[[maybe_unused]] wrapper_x y;
		};

		static_assert(sizeof(point) == sizeof(int) * 2);
		static_assert(sizeof(point) == sizeof(point_x));
		static_assert(std::is_trivially_constructible_v<point>);
		static_assert(std::is_trivially_constructible_v<point_x>);

		template<typename L, typename R>
		constexpr auto can_x = requires { std::declval<const L&>() <= std::declval<const R&>(); };
		static_assert(can_x<point_x, point>);

		constexpr point p{.x = 4200, .y = 13370};

		constexpr point_x px1{.x = {.value = 1234}, .y = {.value = 6789}};
		constexpr point_x px2{.x = {.value = 6789}, .y = {.value = 12340}};

		[[nodiscard]] constexpr auto do_x(const auto& lhs, const auto& rhs) noexcept -> auto
		{
			return lhs <= rhs;
		}

		// point_x <= point_x
		static_assert(std::ranges::all_of(do_x(px1, px2), std::identity{}));
		// point_x <= point
		static_assert(std::ranges::all_of(do_x(px1, p), std::identity{}));
		// point_x <= value
		static_assert(std::ranges::all_of(do_x(px1, 13579), std::identity{}));
	}
}
