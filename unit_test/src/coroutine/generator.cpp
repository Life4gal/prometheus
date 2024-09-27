#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_USE_MODULE
import std;
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

using namespace gal::prometheus;

namespace
{
	using namespace gal::prometheus;
	using namespace coroutine;

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"coroutine.generator"> _ = []
	{
		using namespace unit_test;

		const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		"0~10"_test = []
		{
			auto generator = []() -> Generator<int> { for (int i = 0; i <= 10; ++i) { co_yield i; } }();

			for (int i = 0;
			     const auto each: generator)
			{
				expect(each == value(i)) << fatal;
				++i;
			}
		};

		"0~10 with exception"_test = []
		{
			auto generator = []() -> Generator<int>
			{
				for (int i = 0; i <= 10; ++i)
				{
					if (i == 5) { throw std::runtime_error{"catch me + " + std::to_string(i)}; }

					co_yield i;
				}
			}();

			try
			{
				for (int i = 0;
				     const auto each: generator)
				{
					expect(each == value(i)) << fatal;
					++i;
				}
			}
			catch (const std::runtime_error& e) { expect(e.what() == std::string_view{"catch me + 5"}) << fatal; }
		};

		"0~10 with unique_ptr"_test = []
		{
			// const T&
			{
				auto generator = []() -> Generator<std::unique_ptr<int>> { for (int i = 0; i <= 10; ++i) { co_yield std::make_unique<int>(i); } }();

				for (int i = 0;
				     const auto& each: generator)
				{
					expect(*each == value(i)) << fatal;
					++i;
				}
			}

			// T&&
			{
				auto generator = []() -> Generator<std::unique_ptr<int>> { for (int i = 0; i <= 10; ++i) { co_yield std::make_unique<int>(i); } }();

				for (int i = 0;
				     auto&& each: generator)
				{
					expect(*each == value(i)) << fatal;
					++i;
				}
			}
		};

		"infinite"_test = []
		{
			auto generator = []() -> Generator<unsigned int>
			{
				unsigned int i = 0;
				while (true)
				{
					co_yield i;
					++i;
				}
			}();

			for (unsigned int i = 0;
			     const auto each: generator)
			{
				expect(each == value(i)) << fatal;
				++i;

				if (constexpr unsigned int max = 1024;
					i > max) { break; }
			}
		};

		config().output_level = old_level;
	};
} // namespace
