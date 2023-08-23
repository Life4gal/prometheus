#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY suite test_i18n_iso_639 = []
	{
		"parse"_test = []
		{
			static_assert(ISO639::parse("ab").has_value());
			static_assert(ISO639::parse("ab")->size() == 2);
			static_assert(ISO639::parse("ab")->code() == "ab");
			static_assert(ISO639::parse("ab")->get<0>() == 'a');
			static_assert(ISO639::parse("ab")->get<1>() == 'b');
			static_assert(ISO639::parse("ab")->get<2>() == 0);

			static_assert(ISO639::parse("abc").has_value());
			static_assert(ISO639::parse("abc")->size() == 3);
			static_assert(ISO639::parse("abc")->code() == "abc");
			static_assert(ISO639::parse("abc")->get<0>() == 'a');
			static_assert(ISO639::parse("abc")->get<1>() == 'b');
			static_assert(ISO639::parse("abc")->get<2>() == 'c');

			static_assert(not ISO639::parse("a").has_value());
			static_assert(not ISO639::parse("abcd").has_value());
		};

		"hash"_test = []
		{
			expect((std::hash<ISO639>{}(*ISO639::parse("bc")) != 0_ull) >> fatal);
			expect((std::hash<ISO639>{}(*ISO639::parse("abc")) != 0_ull) >> fatal);
		};
	};
}// namespace
