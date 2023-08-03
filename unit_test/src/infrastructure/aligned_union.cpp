#include <boost/ut.hpp>
#include <prometheus/infrastructure/aligned_union.hpp>
#include <prometheus/macro.hpp>

using namespace boost::ut;
using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_NO_DESTROY suite test_aligned_union = []
	{
		"arithmethic"_test = []
		{
			using union_type = AlignedUnion<int, unsigned, float>;

			union_type u{union_type::constructor_tag<int>{}, 42};
			expect((u.load<int>() == 42_i) >> fatal);

			u.store<unsigned>(123);
			expect((u.load<unsigned>() == 123_u) >> fatal);

			u.store<float>(3.14f);
			expect((u.load<float>() == 3.14_f) >> fatal);
		};

		"pointer"_test = []
		{
			using union_type = AlignedUnion<int*, unsigned*, float*>;
			static_assert(union_type::max_size == sizeof(int*));

			int      value_i = 42;
			unsigned value_u = 123;
			float    value_f = 3.14f;

			auto* pointer_i = &value_i;
			auto* pointer_u = &value_u;
			auto* pointer_f = &value_f;

			union_type u{union_type::constructor_tag<int*>{}, pointer_i};
			expect((u.load<int*>() == pointer_i) >> fatal);

			u.store<unsigned*>(pointer_u);
			expect((u.load<unsigned*>() == pointer_u) >> fatal);

			u.store<float*>(pointer_f);
			expect((u.load<float*>() == pointer_f) >> fatal);
		};

		// fixme: syntax error?
		#if not defined(GAL_PROMETHEUS_COMPILER_MSVC)
		"structure"_test = []
		{
			struct struct1
			{
				int a;
				int b;
				int c;
				int d;
			};

			struct struct2
			{
				std::string string;
			};

			struct struct3
			{
				using data_type = std::array<int, 4>;

				data_type data;
			};

			using union_type = AlignedUnion<struct1, struct2, struct3>;

			union_type u{union_type::constructor_tag<struct1>{}, 1, 2, 3, 4};
			{
				const auto& s1 = u.load<struct1>();

				expect((s1.a == 1_i) >> fatal);
				expect((s1.b == 2_i) >> fatal);
				expect((s1.c == 3_i) >> fatal);
				expect((s1.d == 4_i) >> fatal);
			}

			u.store<struct2>("hello world");
			{
				const auto& s2 = u.load<struct2>();

				expect((s2.string == "hello world") >> fatal);

				// destroy it!
				u.destroy<struct2>();
			}

			u.store<struct3>(struct3::data_type{1, 2, 3, 4});
			{
				const auto& [data] = u.load<struct3>();

				expect((data[0] == 1_i) >> fatal);
				expect((data[1] == 2_i) >> fatal);
				expect((data[2] == 3_i) >> fatal);
				expect((data[3] == 4_i) >> fatal);
			}
		};
		#endif
	};
}