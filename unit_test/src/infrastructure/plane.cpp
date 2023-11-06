#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;

#if __has_cpp_attribute(__cpp_multidimensional_subscript)
	#define PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT 1
#else
#define PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT 0
#endif

namespace
{
	using namespace gal::prometheus;
	using namespace infrastructure;

	using plane_type = Plane<std::uint8_t>;
	using plane_view_type = PlaneView<std::uint8_t>;
	using value_type = plane_type::value_type;
	using size_type = plane_type::size_type;

	constexpr size_type plane_default_width{4};
	constexpr size_type plane_default_height{3};

	constexpr auto make_plane() -> plane_type
	{
		// 0    1   2   3
		// 4    5   6   7
		// 8    9   10  11

		plane_type result{plane_default_width, plane_default_height};

		value_type value{0};
		for (auto& pixel: result) { pixel = value++; }
		return result;
	}

	GAL_PROMETHEUS_NO_DESTROY test::suite<"infrastructure.plane_view"> _1 = []
	{
		using namespace test;

		ignore_pass / "default_constructor"_test = []
		{
			constexpr plane_view_type plane_view{};

			expect(plane_view.empty() == "empty"_b) << fatal;
			expect(plane_view.width() == 0_ull) << fatal;
			expect(plane_view.height() == 0_ull) << fatal;
			expect(plane_view.stride() == 0_ull) << fatal;
		};

		ignore_pass / "convert_from_plane"_test = []
		{
			auto plane      = make_plane();
			auto plane_view = plane_view_type{plane};

			expect(not plane_view.empty() == "not empty"_b) << fatal;
			expect(plane_view.width() == as_ull{plane_default_width}) << fatal;
			expect(plane_view.height() == as_ull{plane_default_height}) << fatal;
			expect(plane_view.stride() == as_ull{plane_default_width}) << fatal;

			value_type value{0};
			for (size_type y = 0; y < plane_default_height; ++y)
			{
				for (size_type x = 0; x < plane_default_width; ++x)
				{
					#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
					expect(plane_view[x, y] == as_ull{value}) << fatal;
					#else
					expect(plane_view(x, y) == as_ull{value}) << fatal;
					#endif

					++value;
				}
			}
		};

		ignore_pass / "convert_to_plane"_test = []
		{
			auto origin_plane      = make_plane();
			auto origin_plane_view = plane_view_type{origin_plane};

			// 1 2
			// 5 6
			auto plane_view = origin_plane_view.sub_view(1, 0, 2, 2);

			expect(not plane_view.empty() == "no empty"_b) << fatal;
			expect(plane_view.width() == 2_ull) << fatal;
			expect(plane_view.height() == 2_ull) << fatal;
			expect(plane_view.stride() == as_ull{plane_default_width}) << fatal;

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			expect(plane_view[0, 0] == 1_ull) << fatal;
			expect(plane_view[1, 0] == 2_ull) << fatal;
			expect(plane_view[0, 1] == 5_ull) << fatal;
			expect(plane_view[1, 1] == 6_ull) << fatal;
			#else
			expect(plane_view(0, 0) == 1_ull) << fatal;
			expect(plane_view(1, 0) == 2_ull) << fatal;
			expect(plane_view(0, 1) == 5_ull) << fatal;
			expect(plane_view(1, 1) == 6_ull) << fatal;
			#endif

			Plane plane{plane_view, plane_type::allocator_type{}};

			expect(not plane.empty() == "no empty"_b) << fatal;
			expect(plane.width() == 2_ull) << fatal;
			expect(plane.height() == 2_ull) << fatal;
			expect(plane.capacity() == as_ull{2 * 2}) << fatal;

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			expect(plane[0, 0] == 1_ull) << fatal;
			expect(plane[1, 0] == 2_ull) << fatal;
			expect(plane[0, 1] == 5_ull) << fatal;
			expect(plane[1, 1] == 6_ull) << fatal;
			#else
			expect(plane(0, 0) == 1_ull) << fatal;
			expect(plane(1, 0) == 2_ull) << fatal;
			expect(plane(0, 1) == 5_ull) << fatal;
			expect(plane(1, 1) == 6_ull) << fatal;
			#endif
		};

		ignore_pass / "construct_from_data"_test = []
		{
			auto plane = make_plane();

			// 0    1   2
			// 4    5   6
			// 8    9   10
			auto plane_view = PlaneView{plane.data(), 3, 3, 4};

			expect(not plane_view.empty() == "not empty"_b) << fatal;
			expect(plane_view.width() == 3_ull) << fatal;
			expect(plane_view.height() == 3_ull) << fatal;
			expect(plane_view.stride() == 4_ull) << fatal;

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			expect(plane_view[0, 0] == 0_ull) << fatal;
			expect(plane_view[1, 0] == 1_ull) << fatal;
			expect(plane_view[2, 0] == 2_ull) << fatal;
			expect(plane_view[0, 1] == 4_ull) << fatal;
			expect(plane_view[1, 1] == 5_ull) << fatal;
			expect(plane_view[2, 1] == 6_ull) << fatal;
			expect(plane_view[0, 2] == 8_ull) << fatal;
			expect(plane_view[1, 2] == 9_ull) << fatal;
			expect(plane_view[2, 2] == 10_ull) << fatal;
			#else
			expect(plane_view(0, 0) == 0_ull) << fatal;
			expect(plane_view(1, 0) == 1_ull) << fatal;
			expect(plane_view(2, 0) == 2_ull) << fatal;
			expect(plane_view(0, 1) == 4_ull) << fatal;
			expect(plane_view(1, 1) == 5_ull) << fatal;
			expect(plane_view(2, 1) == 6_ull) << fatal;
			expect(plane_view(0, 2) == 8_ull) << fatal;
			expect(plane_view(1, 2) == 9_ull) << fatal;
			expect(plane_view(2, 2) == 10_ull) << fatal;
			#endif
		};

		ignore_pass / "copy_assign"_test = []
		{
			const auto plane      = make_plane();
			auto       plane_view = plane_view_type{};

			expect(plane_view.empty() == "empty"_b) << fatal;
			expect(plane_view.width() == 0_ull) << fatal;
			expect(plane_view.height() == 0_ull) << fatal;
			expect(plane_view.stride() == 0_ull) << fatal;
			expect(not plane_view.data() == "nullptr"_b) << fatal;

			plane_view = plane;

			expect(not plane_view.empty() == "not empty"_b) << fatal;
			expect(plane_view.width() == as_ull{plane_default_width}) << fatal;
			expect(plane_view.height() == as_ull{plane_default_height}) << fatal;
			expect(plane_view.stride() == as_ull{plane_default_width}) << fatal;

			value_type value{0};
			for (size_type y = 0; y < plane_default_height; ++y)
			{
				for (size_type x = 0; x < plane_default_width; ++x)
				{
					#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
					expect(plane_view[x, y] == as_ull{value}) << fatal;
					#else
					expect(plane_view(x, y) == as_ull{value}) << fatal;
					#endif

					++value;
				}
			}
		};
	};

	GAL_PROMETHEUS_NO_DESTROY test::suite<"infrastructure.plane"> _2 = []
	{
		using namespace test;

		ignore_pass / "default_constructor"_test = []
		{
			constexpr plane_type plane{};

			expect(plane.empty() == "empty"_b) << fatal;
			expect(plane.width() == 0_ull) << fatal;
			expect(plane.height() == 0_ull) << fatal;
			expect(plane.size() == 0_ull) << fatal;
			expect(plane.capacity() == 0_ull) << fatal;
		};

		ignore_pass / "construct_with_zero_fill"_test = []
		{
			plane_type plane{plane_default_width, plane_default_height};

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			for (auto& each: plane) { expect(each == 0_ull) << fatal; }
		};

		ignore_pass / "copy_construct"_test = []
		{
			const auto origin_plane = make_plane();
			auto       plane        = origin_plane;

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			value_type value{0};
			for (auto& each: plane)
			{
				expect(each == as_ull{value}) << fatal;
				++value;
			}
		};

		ignore_pass / "move_construct"_test = []
		{
			auto origin_plane = make_plane();
			auto plane        = std::move(origin_plane);

			expect(origin_plane.empty() == "empty"_b) << fatal;
			expect(origin_plane.width() == 0_ull) << fatal;
			expect(origin_plane.height() == 0_ull) << fatal;
			expect(origin_plane.size() == 0_ull) << fatal;
			expect(origin_plane.capacity() == 0_ull) << fatal;

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			value_type value{0};
			for (auto& each: plane)
			{
				expect(each == as_ull{value}) << fatal;
				++value;
			}
		};

		ignore_pass / "construct_from_data"_test = []
		{
			auto origin_plane = make_plane();

			// 0    1   2
			// 4    5   6
			// 8    9   10
			Plane plane{origin_plane.data(), 3, 3, 4};

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == 3_ull) << fatal;
			expect(plane.height() == 3_ull) << fatal;
			expect(plane.size() == as_ull{3 * 3}) << fatal;
			expect(plane.capacity() == as_ull{3 * 3}) << fatal;

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			expect(plane[0, 0] == 0_ull) << fatal;
			expect(plane[1, 0] == 1_ull) << fatal;
			expect(plane[2, 0] == 2_ull) << fatal;
			expect(plane[0, 1] == 4_ull) << fatal;
			expect(plane[1, 1] == 5_ull) << fatal;
			expect(plane[2, 1] == 6_ull) << fatal;
			expect(plane[0, 2] == 8_ull) << fatal;
			expect(plane[1, 2] == 9_ull) << fatal;
			expect(plane[2, 2] == 10_ull) << fatal;
			#else
			expect(plane(0, 0) == 0_ull) << fatal;
			expect(plane(1, 0) == 1_ull) << fatal;
			expect(plane(2, 0) == 2_ull) << fatal;
			expect(plane(0, 1) == 4_ull) << fatal;
			expect(plane(1, 1) == 5_ull) << fatal;
			expect(plane(2, 1) == 6_ull) << fatal;
			expect(plane(0, 2) == 8_ull) << fatal;
			expect(plane(1, 2) == 9_ull) << fatal;
			expect(plane(2, 2) == 10_ull) << fatal;
			#endif
		};

		ignore_pass / "copy_assign"_test = []
		{
			const auto origin_plane = make_plane();
			auto       plane        = plane_type{10, 10};

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == 10_ull) << fatal;
			expect(plane.height() == 10_ull) << fatal;
			expect(plane.size() == as_ull{10 * 10}) << fatal;
			expect(plane.capacity() == as_ull{10 * 10}) << fatal;

			plane = origin_plane;

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{10 * 10}) << fatal;

			value_type value{0};
			for (auto& each: plane)
			{
				expect(each == as_ull{value}) << fatal;
				++value;
			}
		};

		ignore_pass / "move_assign"_test = []
		{
			auto origin_plane = make_plane();
			auto plane        = plane_type{10, 10};

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == 10_ull) << fatal;
			expect(plane.height() == 10_ull) << fatal;
			expect(plane.size() == as_ull{10 * 10}) << fatal;
			expect(plane.capacity() == as_ull{10 * 10}) << fatal;

			plane = std::move(origin_plane);

			expect(origin_plane.empty() == "empty"_b) << fatal;
			expect(origin_plane.width() == 0_ull) << fatal;
			expect(origin_plane.height() == 0_ull) << fatal;
			expect(origin_plane.size() == 0_ull) << fatal;
			expect(origin_plane.capacity() == 0_ull) << fatal;

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			value_type value{0};
			for (auto& each: plane)
			{
				expect(each == as_ull{value}) << fatal;
				++value;
			}
		};

		ignore_pass / "shrink_to_fit"_test = []
		{
			const auto origin_plane = make_plane();
			auto       plane        = plane_type{10, 10};

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == 10_ull) << fatal;
			expect(plane.height() == 10_ull) << fatal;
			expect(plane.size() == as_ull{10 * 10}) << fatal;
			expect(plane.capacity() == as_ull{10 * 10}) << fatal;

			plane = origin_plane;

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{10 * 10}) << fatal;

			plane.shrink_to_fit();

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;
		};

		ignore_pass / "clear"_test = []
		{
			auto plane = make_plane();

			expect(not plane.empty() == "not empty"_b) << fatal;
			expect(plane.width() == as_ull{plane_default_width}) << fatal;
			expect(plane.height() == as_ull{plane_default_height}) << fatal;
			expect(plane.size() == as_ull{plane_default_width * plane_default_height}) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			plane.clear();

			expect(plane.empty() == "empty"_b) << fatal;
			expect(plane.width() == 0_ull) << fatal;
			expect(plane.height() == 0_ull) << fatal;
			expect(plane.size() == 0_ull) << fatal;
			expect(plane.capacity() == as_ull{plane_default_width * plane_default_height}) << fatal;

			plane.shrink_to_fit();

			expect(plane.empty() == "empty"_b) << fatal;
			expect(plane.width() == 0_ull) << fatal;
			expect(plane.height() == 0_ull) << fatal;
			expect(plane.size() == 0_ull) << fatal;
			expect(plane.capacity() == 0_ull) << fatal;
		};

		ignore_pass / "sub_plane"_test = []
		{
			auto origin_plane = make_plane();

			// 1 2
			// 5 6
			auto plane = origin_plane.sub_plane(1, 0, 2, 2);

			expect(not plane.empty() == "no empty"_b) << fatal;
			expect(plane.width() == 2_ull) << fatal;
			expect(plane.height() == 2_ull) << fatal;
			expect(plane.size() == as_ull{2 * 2}) << fatal;
			expect(plane.capacity() == as_ull{2 * 2}) << fatal;

			#if PLANE_SUBSCRIPT_OPERATOR_SUBSCRIPT
			expect(plane[0, 0] == 1_ull) << fatal;
			expect(plane[1, 0] == 2_ull) << fatal;
			expect(plane[0, 1] == 5_ull) << fatal;
			expect(plane[1, 1] == 6_ull) << fatal;
			#else
			expect(plane(0, 0) == 1_ull) << fatal;
			expect(plane(1, 0) == 2_ull) << fatal;
			expect(plane(0, 1) == 5_ull) << fatal;
			expect(plane(1, 1) == 6_ull) << fatal;
			#endif
		};
	};
}// namespace
