#include <unit_test/unit_test.hpp>
// functional::function_ref
#include <functional/functional.hpp>

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"utility.function_ref"> _ = []
	{
		using namespace unit_test;
		using namespace functional;

		"functor"_test = []
		{
			struct functor
			{
				constexpr auto operator()(const int a, const int b) const noexcept -> int { return a + b; }

				constexpr auto operator()(int& a) const noexcept -> void { a = 42; }
			};

			functor f{};

			const auto a = take<int, int> + to<int> + f;
			expect(a(42, 1337) == value(42 + 1337)) << fatal;

			const auto b = take<int, short> + to<int> + f;
			expect(b(42, 1337) == value(42 + 1337)) << fatal;

			int v = 1337;
			expect(v == 1337_i) << fatal;

			const auto c = take<int&> + to<void> + f;
			c(v);
			expect(v == 42_i) << fatal;

			v = 1337;
			const auto d = take<int&, int> + to<void> + f;
			// note: call functor::operator()(const int a, const int b)
			d(v, 123);
			expect(v == 1337_i) << fatal;
		};

		"function pointer"_test = []
		{
			const auto f = +[](const int a, const int b) noexcept -> int { return a + b; };

			const auto a = to<int> + take<int, int> + f;
			expect(a(42, 1337) == value(42 + 1337)) << fatal;

			const auto b = to<int> + take<int, short> + f;
			expect(b(42, 1337) == value(42 + 1337)) << fatal;

			// compatible
			const auto c = to<void> + take<int, short> + f;
			c(42, 1337);
		};

		"lambda"_test = []
		{
			{
				const auto f = [](const int a, const int b) noexcept -> int { return a + b; };

				const auto a = take<int, int> + to<int> + f;
				expect(a(42, 1337) == value(42 + 1337)) << fatal;

				const auto b = take<int, short> + to<int> + f;
				expect(b(42, 1337) == value(42 + 1337)) << fatal;

				// compatible
				const auto c = take<int, short> + to<void> + f;
				c(42, 1337);
			}
			{
				int i = 42;
				const auto f = [&i](const int a, const int b) noexcept -> int { return i + a + b; };

				const auto a = take<int, int> + to<int> + f;
				expect(a(42, 1337) == value(i + 42 + 1337)) << fatal;

				const auto b = take<int, short> + to<int> + f;
				expect(b(42, 1337) == value(i + 42 + 1337)) << fatal;

				// compatible
				const auto c = take<int, short> + to<void> + f;
				c(42, 1337);
			}
		};

		"member function"_test = []
		{
			class Foo
			{
				int _{0};

			public:
				[[nodiscard]] constexpr auto bar(const int a, const int b) noexcept -> int // NOLINT
				{
					_ = a + b;

					return _;
				}
			};

			Foo foo{};

			const auto a = ref<int, Foo&, int, int>([](Foo& f, const int v1, const int v2) noexcept -> int { return f.bar(v1, v2); });
			expect(a(foo, 42, 1337) == value(42 + 1337)) << fatal;

			auto function_pointer = std::mem_fn(&Foo::bar);
			const auto b = ref<int, Foo&, int, int>(function_pointer);
			expect(b(foo, 42, 1337) == value(42 + 1337)) << fatal;
		};
	};
}
