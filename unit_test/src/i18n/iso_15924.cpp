#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;
import gal.prometheus.infrastructure;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY suite test_i18n_iso_15924 = []
	{
		"parse"_test = []
		{
			// adlm / 166
			static_assert(ISO15924::parse("adlm").has_value());
			static_assert(ISO15924::parse("adlm")->code4() == "Adlm");
			static_assert(ISO15924::parse("adlm")->number() == 166);

			static_assert(ISO15924::parse("Adlm").has_value());
			static_assert(ISO15924::parse("Adlm")->code4() == "Adlm");
			static_assert(ISO15924::parse("Adlm")->number() == 166);
			static_assert(ISO15924::parse("aDlm").has_value());
			static_assert(ISO15924::parse("aDlm")->code4() == "Adlm");
			static_assert(ISO15924::parse("aDlm")->number() == 166);
			static_assert(ISO15924::parse("adLm").has_value());
			static_assert(ISO15924::parse("adLm")->code4() == "Adlm");
			static_assert(ISO15924::parse("adLm")->number() == 166);
			static_assert(ISO15924::parse("adlM").has_value());
			static_assert(ISO15924::parse("adlM")->code4() == "Adlm");
			static_assert(ISO15924::parse("adlM")->number() == 166);

			static_assert(ISO15924::parse("166").has_value());
			static_assert(ISO15924::parse("166")->code4() == "Adlm");
			static_assert(ISO15924::parse("166")->number() == 166);

			static_assert(ISO15924::parse("0")->number() == 0);
			static_assert(ISO15924::parse("01")->number() == 1);
			static_assert(ISO15924::parse("10")->number() == 10);
			static_assert(ISO15924::parse("100")->number() == 100);
			static_assert(ISO15924::parse("010")->number() == 10);
			static_assert(ISO15924::parse("999")->number() == 999);
			static_assert(not ISO15924::parse("1000").has_value());

			static_assert(not ISO15924::parse("").has_value());
			static_assert(not ISO15924::parse("?").has_value());
			static_assert(not ISO15924::parse("??").has_value());
			static_assert(not ISO15924::parse("???").has_value());
			static_assert(not ISO15924::parse("????").has_value());
		};

		"hash"_test = []
		{
			expect((std::hash<ISO15924>{}(*ISO15924::parse("adlm")) == std::hash<ISO15924>{}(*ISO15924::parse("166"))) >> fatal);
			//
		};
	};
}// namespace
