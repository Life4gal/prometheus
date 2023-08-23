#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY suite test_i18n_iso_3166 = []
	{
		"parse"_test = []
		{
			// AF / AFG / 4
			static_assert(ISO3166::parse("AF").has_value());
			static_assert(ISO3166::parse("AF")->code2() == "AF");
			static_assert(ISO3166::parse("AF")->code3() == "AFG");
			static_assert(ISO3166::parse("AF")->number() == 4);

			static_assert(ISO3166::parse("AFG").has_value());
			static_assert(ISO3166::parse("AFG")->code2() == "AF");
			static_assert(ISO3166::parse("AFG")->code3() == "AFG");
			static_assert(ISO3166::parse("AFG")->number() == 4);

			static_assert(ISO3166::parse("4").has_value());
			static_assert(ISO3166::parse("4")->code2() == "AF");
			static_assert(ISO3166::parse("4")->code3() == "AFG");
			static_assert(ISO3166::parse("4")->number() == 4);

			static_assert(ISO3166::parse("0")->number() == 0);
			static_assert(ISO3166::parse("01")->number() == 1);
			static_assert(ISO3166::parse("10")->number() == 10);
			static_assert(ISO3166::parse("100")->number() == 100);
			static_assert(ISO3166::parse("010")->number() == 10);
			static_assert(ISO3166::parse("999")->number() == 999);
			static_assert(not ISO3166::parse("1000").has_value());

			static_assert(not ISO3166::parse("").has_value());
			static_assert(not ISO3166::parse("?").has_value());
			static_assert(not ISO3166::parse("??").has_value());
			static_assert(not ISO3166::parse("???").has_value());
			static_assert(not ISO3166::parse("????").has_value());
		};

		"hash"_test = []
		{
			expect((std::hash<ISO3166>{}(*ISO3166::parse("AF")) == std::hash<ISO3166>{}(*ISO3166::parse("AFG"))) >> fatal);
			expect((std::hash<ISO3166>{}(*ISO3166::parse("AF")) == std::hash<ISO3166>{}(*ISO3166::parse("4"))) >> fatal);
			expect((std::hash<ISO3166>{}(*ISO3166::parse("AFG")) == std::hash<ISO3166>{}(*ISO3166::parse("4"))) >> fatal);
		};
	};
}// namespace
