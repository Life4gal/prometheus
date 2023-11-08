#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY test::suite<"i18n.iso_15924"> _ = []
	{
		using namespace test;

		ignore_pass / "parse"_test = []
		{
			// adlm / 166
			expect(constant<ISO15924::parse("adlm").has_value()> and ISO15924::parse("adlm").has_value() == "ISO15924::parse(\"adlm\").has_value()"_b);
			expect(constant<ISO15924::parse("adlm")->code4() == "Adlm"_s> and ISO15924::parse("adlm")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("adlm")->number() == 166_auto> and ISO15924::parse("adlm")->number() == 166_auto);

			expect(constant<ISO15924::parse("Adlm").has_value()> and ISO15924::parse("Adlm").has_value() == "ISO15924::parse(\"Adlm\").has_value()"_b);
			expect(constant<ISO15924::parse("Adlm")->code4() == "Adlm"_s> and ISO15924::parse("Adlm")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("Adlm")->number() == 166_auto> and ISO15924::parse("Adlm")->number() == 166_auto);
			expect(constant<ISO15924::parse("aDlm").has_value()> and ISO15924::parse("aDlm").has_value() == "ISO15924::parse(\"aDlm\").has_value()"_b);
			expect(constant<ISO15924::parse("aDlm")->code4() == "Adlm"_s> and ISO15924::parse("aDlm")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("aDlm")->number() == 166_auto> and ISO15924::parse("aDlm")->number() == 166_auto);
			expect(constant<ISO15924::parse("adLm").has_value()> and ISO15924::parse("adLm").has_value() == "ISO15924::parse(\"adLm\").has_value()"_b);
			expect(constant<ISO15924::parse("adLm")->code4() == "Adlm"_s> and ISO15924::parse("adLm")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("adLm")->number() == 166_auto> and ISO15924::parse("adLm")->number() == 166_auto);
			expect(constant<ISO15924::parse("adlM").has_value()> and ISO15924::parse("adlM").has_value() == "ISO15924::parse(\"adlM\").has_value()"_b);
			expect(constant<ISO15924::parse("adlM")->code4() == "Adlm"_s> and ISO15924::parse("adlM")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("adlM")->number() == 166_auto> and ISO15924::parse("adlM")->number() == 166_auto);

			expect(constant<ISO15924::parse("166").has_value()> and ISO15924::parse("166").has_value() == "ISO15924::parse(\"166\").has_value()"_b);
			expect(constant<ISO15924::parse("166")->code4() == "Adlm"_s> and ISO15924::parse("166")->code4() == "Adlm"_s);
			expect(constant<ISO15924::parse("166")->number() == 166_auto> and ISO15924::parse("166")->number() == 166_auto);

			expect(constant<ISO15924::parse("0")->number() == 0_auto> and ISO15924::parse("0")->number() == 0_auto);
			expect(constant<ISO15924::parse("01")->number() == 1_auto> and ISO15924::parse("01")->number() == 1_auto);
			expect(constant<ISO15924::parse("10")->number() == 10_auto> and ISO15924::parse("10")->number() == 10_auto);
			expect(constant<ISO15924::parse("100")->number() == 100_auto> and ISO15924::parse("100")->number() == 100_auto);
			expect(constant<ISO15924::parse("010")->number() == 10_auto> and ISO15924::parse("010")->number() == 10_auto);
			expect(constant<ISO15924::parse("999")->number() == 999_auto> and ISO15924::parse("999")->number() == 999_auto);
			expect(constant<not ISO15924::parse("1000").has_value()> and ISO15924::parse("1000").has_value() != "not ISO15924::parse(\"1000\").has_value()"_b);

			expect(constant<not ISO15924::parse("").has_value()> and ISO15924::parse("").has_value() != "not ISO15924::parse(\"\").has_value()"_b);
			expect(constant<not ISO15924::parse("?").has_value()> and ISO15924::parse("?").has_value() != "not ISO15924::parse(\"?\").has_value()"_b);
			expect(constant<not ISO15924::parse("??").has_value()> and ISO15924::parse("??").has_value() != "not ISO15924::parse(\"??\").has_value()"_b);
			expect(constant<not ISO15924::parse("???").has_value()> and ISO15924::parse("???").has_value() != "not ISO15924::parse(\"???\").has_value()"_b);
			expect(constant<not ISO15924::parse("????").has_value()> and ISO15924::parse("????").has_value() != "not ISO15924::parse(\"????\").has_value()"_b);
		};

		ignore_pass / "hash"_test = []
		{
			expect(that % std::hash<ISO15924>{}(*ISO15924::parse("adlm")) == std::hash<ISO15924>{}(*ISO15924::parse("166"))) << fatal;
			//
		};
	};
}// namespace
