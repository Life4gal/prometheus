#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.utility;

namespace
{
	using namespace gal::prometheus;
	using namespace utility;

	class Foo
	{
	public:
		Foo() noexcept = default;

		Foo(const Foo& other)                        = default;
		auto operator=(const Foo& other) -> Foo&     = default;
		Foo(Foo&& other) noexcept                    = default;
		auto operator=(Foo&& other) noexcept -> Foo& = default;

		virtual ~Foo() noexcept = default;

		[[nodiscard]] virtual auto answer() const noexcept -> int { return 42; }
	};

	class Bar final : public Foo
	{
	public:
		#if defined(GAL_PROMETHEUS_COMPILER_GNU)
		// error: two or more data types in declaration of `type name`
		[[nodiscard]] int answer() const noexcept override { return 1337; }// NOLINT
		#else
		[[nodiscard]] auto answer() const noexcept -> int override { return 1337; }
		#endif
	};

	GAL_PROMETHEUS_NO_DESTROY test::suite<"utility.cast"> _ = []
	{
		using namespace test;

		ignore_pass / "up_cast_reference"_test = []
		{
			Bar b{};

			expect(b.answer() == 1337_i) << fatal;

			auto&       f1 = up_cast<Foo&>(b);// NOLINT
			const auto& f2 = up_cast<const Foo&>(b);

			expect(f1.answer() == 1337_i) << fatal;
			expect(f2.answer() == 1337_i) << fatal;

			auto&       b1 = up_cast<Bar&>(b);// NOLINT
			const auto& b2 = up_cast<const Bar&>(b);

			expect(b1.answer() == 1337_i) << fatal;
			expect(b2.answer() == 1337_i) << fatal;
		};

		ignore_pass / "up_cast_pointer"_test = []
		{
			Bar b{};

			expect(b.answer() == 1337_i) << fatal;

			auto*       f1 = up_cast<Foo*>(&b);// NOLINT
			const auto* f2 = up_cast<const Foo*>(&b);

			expect(f1->answer() == 1337_i) << fatal;
			expect(f2->answer() == 1337_i) << fatal;

			auto*       b1 = up_cast<Bar*>(&b);// NOLINT
			const auto* b2 = up_cast<const Bar*>(&b);

			expect(b1->answer() == 1337_i) << fatal;
			expect(b2->answer() == 1337_i) << fatal;
		};

		ignore_pass / "up_cast_nullptr"_test = []
		{
			Bar* b = nullptr;

			auto*       f1 = up_cast<Foo*>(b);// NOLINT
			const auto* f2 = up_cast<const Foo*>(b);

			expect(f1 == as<Foo*>{nullptr}) << fatal;
			expect(f2 == as{b}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr

			auto*       b1 = up_cast<Bar*>(b);// NOLINT
			const auto* b2 = up_cast<const Bar*>(b);

			expect(b1 == as{b}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr
			expect(b2 == as{b}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr
		};

		ignore_pass / "down_cast_reference"_test = []
		{
			Bar  b{};
			Foo& f = b;

			expect(f.answer() == 1337_i) << fatal;

			auto&       f1 = down_cast<Foo&>(f);// NOLINT
			const auto& f2 = down_cast<const Foo&>(f);

			expect(f1.answer() == 1337_i) << fatal;
			expect(f2.answer() == 1337_i) << fatal;

			auto&       b1 = down_cast<Bar&>(f);// NOLINT
			const auto& b2 = down_cast<const Bar&>(f);

			expect(b1.answer() == 1337_i) << fatal;
			expect(b2.answer() == 1337_i) << fatal;
		};

		ignore_pass / "down_cast_pointer"_test = []
		{
			Bar  b{};
			Foo& f = b;

			expect(b.answer() == 1337_i) << fatal;

			auto*       f1 = down_cast<Foo*>(&f);// NOLINT
			const auto* f2 = down_cast<const Foo*>(&f);

			expect(f1->answer() == 1337_i) << fatal;
			expect(f2->answer() == 1337_i) << fatal;

			auto*       b1 = down_cast<Bar*>(&f);// NOLINT
			const auto* b2 = down_cast<const Bar*>(&f);

			expect(b1->answer() == 1337_i) << fatal;
			expect(b2->answer() == 1337_i) << fatal;
		};

		ignore_pass / "down_cast_nullptr"_test = []
		{
			Foo* f = nullptr;

			auto*       f1 = down_cast<Foo*>(f);// NOLINT
			const auto* f2 = down_cast<const Foo*>(f);

			expect(f1 == as<Foo*>{nullptr}) << fatal;
			expect(f2 == as{f}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr

			auto*       b1 = down_cast<Bar*>(f);// NOLINT
			const auto* b2 = down_cast<const Bar*>(f);

			expect(b1 == as{f}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr
			expect(b2 == as{f}) << fatal;// equal to as<Foo*>{nullptr} / as<Bar*>{nullptr} / nullptr
		};
	};
}// namespace
