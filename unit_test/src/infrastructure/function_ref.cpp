#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace infrastructure;

	GAL_PROMETHEUS_NO_DESTROY suite test_infrastructure_function_ref = []
	{
		"functor"_test = []
		{
			struct functor
			{
				constexpr auto operator()(const int a, const int b) const noexcept -> int { return a + b; }

				constexpr auto operator()(int& a) const noexcept -> void { a = 42; }
			};

			functor f{};

			FunctionRef<int(int, int)> a{f};
			expect((a(42, 1337) == _i{42 + 1337}) >> fatal);

			FunctionRef<int(int, short)> b{f};
			expect((b(42, 1337) == _i{42 + 1337}) >> fatal);

			int v = 1337;
			expect((v == 1337_i) >> fatal);

			FunctionRef<void(int&)> c{f};
			c(v);
			expect((v == 42_i) >> fatal);

			v = 1337;
			FunctionRef<void(int&, int)> d{f};
			// note: call functor::operator()(const int a, const int b)
			d(v, 123);
			expect((v == 1337_i) >> fatal);
		};

		"function pointer"_test = []
		{
			const auto f = +[](const int a, const int b) noexcept -> int { return a + b; };

			FunctionRef<int(int, int)> a{f};
			expect((a(42, 1337) == _i{42 + 1337}) >> fatal);

			FunctionRef<int(int, short)> b{f};
			expect((b(42, 1337) == _i{42 + 1337}) >> fatal);

			// compatible
			FunctionRef<void(int, short)> c{f};
			c(42, 1337);
		};

		"lambda"_test = []
		{
			{
				const auto f = [](const int a, const int b) noexcept -> int { return a + b; };

				FunctionRef<int(int, int)> a{f};
				expect((a(42, 1337) == _i{42 + 1337}) >> fatal);

				FunctionRef<int(int, short)> b{f};
				expect((b(42, 1337) == _i{42 + 1337}) >> fatal);

				// compatible
				FunctionRef<void(int, short)> c{f};
				c(42, 1337);
			}
			{
				int        i = 42;
				const auto f = [&i](const int a, const int b) noexcept -> int { return i + a + b; };

				FunctionRef<int(int, int)> a{f};
				expect((a(42, 1337) == _i{i + 42 + 1337}) >> fatal);

				FunctionRef<int(int, short)> b{f};
				expect((b(42, 1337) == _i{i + 42 + 1337}) >> fatal);

				// compatible
				FunctionRef<void(int, short)> c{f};
				c(42, 1337);
			}
		};

		"member function"_test = []
		{
			class Foo
			{
				int _{0};

			public:
				[[nodiscard]] constexpr auto bar(const int a, const int b) noexcept -> int// NOLINT
				{
					_ = a + b;

					return _;
				}
			};

			Foo foo{};

			FunctionRef<int(Foo&, int, int)> a{[](Foo& f, const int v1, const int v2) noexcept -> int { return f.bar(v1, v2); }};
			expect((a(foo, 42, 1337) == _i{42 + 1337}) >> fatal);

			auto                             function_pointer = std::mem_fn(&Foo::bar);
			FunctionRef<int(Foo&, int, int)> b{function_pointer};
			expect((b(foo, 42, 1337) == _i{42 + 1337}) >> fatal);
		};
	};
}
