#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY test::suite<"i18n.iso_3166"> _ = []
	{
		using namespace test;

		ignore_pass / "parse"_test = []
		{
			// AF / AFG / 4
			expect(constant<ISO3166::parse("AF").has_value()> and ISO3166::parse("AF").has_value() == "ISO3166::parse(\"AF\").has_value()"_b);
			expect(constant<ISO3166::parse("AF")->code2() == "AF"_s> and ISO3166::parse("AF")->code2() == "AF"_s);
			expect(constant<ISO3166::parse("AF")->code3() == "AFG"_s> and ISO3166::parse("AF")->code3() == "AFG"_s);
			expect(constant<ISO3166::parse("AF")->number() == 4_auto> and ISO3166::parse("AF")->number() == 4_auto);

			expect(constant<ISO3166::parse("AFG").has_value()> and ISO3166::parse("AFG").has_value() == "ISO3166::parse(\"AFG\").has_value()"_b);
			expect(constant<ISO3166::parse("AFG")->code2() == "AF"_s> and ISO3166::parse("AFG")->code2() == "AF"_s);
			expect(constant<ISO3166::parse("AFG")->code3() == "AFG"_s> and ISO3166::parse("AFG")->code3() == "AFG"_s);
			expect(constant<ISO3166::parse("AFG")->number() == 4_auto> and ISO3166::parse("AFG")->number() == 4_auto);

			expect(constant<ISO3166::parse("4").has_value()> and ISO3166::parse("4").has_value() == "ISO3166::parse(\"4\").has_value()"_b);
			expect(constant<ISO3166::parse("4")->code2() == "AF"_s> and ISO3166::parse("4")->code2() == "AF"_s);
			expect(constant<ISO3166::parse("4")->code3() == "AFG"_s> and ISO3166::parse("4")->code3() == "AFG"_s);
			expect(constant<ISO3166::parse("4")->number() == 4_auto> and ISO3166::parse("4")->number() == 4_auto);

			expect(constant<ISO3166::parse("0")->number() == 0_auto> and ISO3166::parse("0")->number() == 0_auto);
			expect(constant<ISO3166::parse("01")->number() == 1_auto> and ISO3166::parse("01")->number() == 1_auto);
			expect(constant<ISO3166::parse("10")->number() == 10_auto> and ISO3166::parse("10")->number() == 10_auto);
			expect(constant<ISO3166::parse("100")->number() == 100_auto> and ISO3166::parse("100")->number() == 100_auto);
			expect(constant<ISO3166::parse("010")->number() == 10_auto> and ISO3166::parse("010")->number() == 10_auto);
			expect(constant<ISO3166::parse("999")->number() == 999_auto> and ISO3166::parse("999")->number() == 999_auto);
			expect(constant<not ISO3166::parse("1000").has_value()> and ISO3166::parse("1000").has_value() != "not ISO3166::parse(\"1000\").has_value()"_b);

			expect(constant<not ISO3166::parse("").has_value()> and ISO3166::parse("").has_value() != "not ISO3166::parse(\"\").has_value()"_b);
			expect(constant<not ISO3166::parse("?").has_value()> and ISO3166::parse("?").has_value() != "not ISO3166::parse(\"?\").has_value()"_b);
			expect(constant<not ISO3166::parse("??").has_value()> and ISO3166::parse("??").has_value() != "not ISO3166::parse(\"??\").has_value()"_b);
			expect(constant<not ISO3166::parse("???").has_value()> and ISO3166::parse("???").has_value() != "not ISO3166::parse(\"???\").has_value()"_b);
			expect(constant<not ISO3166::parse("????").has_value()> and ISO3166::parse("????").has_value() != "not ISO3166::parse(\"????\").has_value()"_b);
		};

		ignore_pass / "hash"_test = []
		{
			expect(that % std::hash<ISO3166>{}(*ISO3166::parse("AF")) == std::hash<ISO3166>{}(*ISO3166::parse("AFG"))) << fatal;
			expect(that % std::hash<ISO3166>{}(*ISO3166::parse("AF")) == std::hash<ISO3166>{}(*ISO3166::parse("4"))) << fatal;
			expect(that % std::hash<ISO3166>{}(*ISO3166::parse("AFG")) == std::hash<ISO3166>{}(*ISO3166::parse("4"))) << fatal;
		};
	};
}// namespace
