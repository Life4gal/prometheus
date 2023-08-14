#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.coroutine;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace coroutine;

	GAL_PROMETHEUS_NO_DESTROY suite test_coroutine_generator = []
	{
		"0~10"_test = []
		{
			auto generator = []() -> Generator<int> { for (int i = 0; i <= 10; ++i) { co_yield i; } }();

			for (int       i = 0;
				const auto each: generator)
			{
				expect((each == _i{i}) >> fatal);
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
				for (int       i = 0;
					const auto each: generator)
				{
					expect((each == _i{i}) >> fatal);
					++i;
				}
			}
			catch (const std::runtime_error& e) { expect((e.what() == std::string_view{"catch me + 5"}) >> fatal); }
		};

		"0~10 with unique_ptr"_test = []
		{
			// const T&
			{
				auto generator = []() -> Generator<std::unique_ptr<int>> { for (int i = 0; i <= 10; ++i) { co_yield std::make_unique<int>(i); } }();

				for (int        i = 0;
					const auto& each: generator)
				{
					expect((*each == _i{i}) >> fatal);
					++i;
				}
			}

			// T&&
			{
				auto generator = []() -> Generator<std::unique_ptr<int>> { for (int i = 0; i <= 10; ++i) { co_yield std::make_unique<int>(i); } }();

				for (int   i = 0;
					auto&& each: generator)
				{
					expect((*each == _i{i}) >> fatal);
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

			for (int       current = 0;
				const auto each: generator)
			{
				expect((each == _i{current}) >> fatal);
				++current;

				if (constexpr int max = 1024;
					current > max) { break; }
			}
		};
	};
}// namespace
