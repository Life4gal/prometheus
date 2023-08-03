#include <boost/ut.hpp>
#include <prometheus/infrastructure/utility.hpp>
#include <prometheus/macro.hpp>

using namespace boost::ut;
using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_NO_DESTROY suite test_utility = []
	{
		"static_assert"_test = []
		{
			static_assert(functor::all(1, 2, 3, 4, 5));
			static_assert(functor::all([](const auto i) -> bool { return i != 0; }, 1, 2, 3, 4, 5));

			static_assert(functor::any(1, 2, 3, 4, 5));
			static_assert(functor::any([](const auto i) -> bool { return i != 0; }, 0, 0, 1, 0, 0));

			static_assert(functor::none(0, 0, 0, 0, 0));
			static_assert(functor::none([](const auto i) -> bool { return i % 2 == 0; }, 1, 3, 5, 7, 9));

			static_assert(functor::max(sizeof(char), sizeof(int), sizeof(float), sizeof(double), sizeof(std::string)) == sizeof(std::string));
			static_assert(functor::min(sizeof(char), sizeof(int), sizeof(float), sizeof(double), sizeof(std::string)) == sizeof(char));
		};
	};
}
