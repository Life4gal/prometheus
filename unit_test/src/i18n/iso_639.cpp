#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY suite<"i18n.iso_639"> _ = []
	{
		"parse"_test = []
		{
			expect(constant<ISO639::parse("ab").has_value()> and ISO639::parse("ab").has_value() == "ISO639::parse(\"ab\").has_value()"_b);
			expect(constant<ISO639::parse("ab")->size() == 2_auto> and ISO639::parse("ab")->size() == 2_auto);
			expect(constant<ISO639::parse("ab")->code() == "ab"_s> and ISO639::parse("ab")->code() == "ab"_s);
			expect(constant<ISO639::parse("ab")->get<0>() == "a"_c> and ISO639::parse("ab")->get<0>() == "a"_c);
			expect(constant<ISO639::parse("ab")->get<1>() == "b"_c> and ISO639::parse("ab")->get<1>() == "b"_c);
			expect(constant<static_cast<int>(ISO639::parse("ab")->get<2>()) == 0_auto> and static_cast<int>(ISO639::parse("ab")->get<2>()) == 0_auto);

			expect(constant<ISO639::parse("abc").has_value()> and ISO639::parse("abc").has_value() == "ISO639::parse(\"abc\").has_value()"_b);
			expect(constant<ISO639::parse("abc")->size() == 3_auto> and ISO639::parse("abc")->size() == 3_auto);
			expect(constant<ISO639::parse("abc")->code() == "abc"_s> and ISO639::parse("abc")->code() == "abc"_s);
			expect(constant<ISO639::parse("abc")->get<0>() == "a"_c> and ISO639::parse("abc")->get<0>() == "a"_c);
			expect(constant<ISO639::parse("abc")->get<1>() == "b"_c> and ISO639::parse("abc")->get<1>() == "b"_c);
			expect(constant<ISO639::parse("abc")->get<2>() == "c"_c> and ISO639::parse("abc")->get<2>() == "c"_c);

			expect(constant<not ISO639::parse("a").has_value()> and ISO639::parse("a").has_value() != "not ISO639::parse(\"a\").has_value()"_b);
			expect(constant<not ISO639::parse("abcd").has_value()> and ISO639::parse("abcd").has_value() != "not ISO639::parse(\"abcd\").has_value()"_b);
		};

		"hash"_test = []
		{
			expect(std::hash<ISO639>{}(*ISO639::parse("bc")) != 0_auto) << fatal;
			expect(std::hash<ISO639>{}(*ISO639::parse("abc")) != 0_auto) << fatal;
		};
	};
}// namespace
