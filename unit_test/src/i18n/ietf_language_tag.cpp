#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY suite test_i18n_ietf_language_tag = []
	{
		"parse"_test = []
		{
			// fixme: For some (unknown) reason, compile-time constants always produce the wrong result (a default constructed value).

			// aa-Latn-ET
			{
				// language
				static_assert(IETFLanguageTag::parse("aa").has_value());
				// static_assert(IETFLanguageTag::parse("aa")->operator std::string() == "aa");

				const std::string string{"aa"};
				const auto        result = IETFLanguageTag::parse(string);

				expect((result.has_value()) >> fatal);
				expect((result->operator std::string() == "aa"_sv) >> fatal);
				expect((std::format("{}", *result) == "aa"_sv) >> fatal);
			}
			{
				// language + region
				{
					// static_assert(IETFLanguageTag::parse("aa-ET").has_value());
					// static_assert(IETFLanguageTag::parse("aa-ET")->operator std::string() == "aa-ET");

					const std::string string{"aa-ET"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
			expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
			expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
				}
				{
					const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number());
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
				}
			}
			{
				// language + script + region
				// static_assert(IETFLanguageTag::parse("aa-Latn-ET").has_value());
				// static_assert(IETFLanguageTag::parse("aa-Latn-ET")->operator std::string() == "aa-Latn-ET");

				const std::string string{"aa-Latn-ET"};
				const auto        result = IETFLanguageTag::parse(string);

				expect((result.has_value()) >> fatal);
				expect((result->operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((std::format("{}", *result) == "aa-Latn-ET"_sv) >> fatal);
			}
			{
				// Optional variant sub-tags, separated by hyphens, each composed of five to eight letters, or of four characters starting with a digit
				{
					const std::string string{"aa-polyton"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa"_sv) >> fatal);
				}
				{
					{
						const std::string string{"aa-ET-polyton"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
					{
						const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-polyton";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
				}
				{
					const std::string string{"aa-Latn-ET-polyton"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa-Latn-ET"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa-Latn-ET"_sv) >> fatal);
				}
			}
			{
				// Optional extension sub-tags, separated by hyphens, each composed of a single character, except the letter x, and a hyphen followed by one or more sub-tags of two to eight characters each, separated by hyphens
				{
					const std::string string{"aa-u-cu-usd"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa"_sv) >> fatal);
				}
				{
					{
						const std::string string{"aa-ET-u-cu-usd"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
					{
						const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-u-cu-usd";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
				}
				{
					const std::string string{"aa-Latn-ET-u-cu-usd"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa-Latn-ET"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa-Latn-ET"_sv) >> fatal);
				}
			}
			{
				// An optional private-use subtag, composed of the letter x and a hyphen followed by sub-tags of one to eight characters each, separated by hyphens
				{
					const std::string string{"aa-x-private"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa"_sv) >> fatal);
				}
				{
					{
						const std::string string{"aa-ET-x-private"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
					{
						const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-x-private";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "aa-ET"_sv) >> fatal);
						expect((std::format("{}", *result) == "aa-ET"_sv) >> fatal);
					}
				}
				{
					const std::string string{"aa-Latn-ET-x-private"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "aa-Latn-ET"_sv) >> fatal);
					expect((std::format("{}", *result) == "aa-Latn-ET"_sv) >> fatal);
				}
			}
			// haz-Arab-AF
			{
				// language
				static_assert(IETFLanguageTag::parse("haz").has_value());
				// static_assert(IETFLanguageTag::parse("haz")->operator std::string() == "haz");

				const std::string string{"haz"};
				const auto        result = IETFLanguageTag::parse(string);

				expect((result.has_value()) >> fatal);
				expect((result->operator std::string() == "haz"_sv) >> fatal);
				expect((std::format("{}", *result) == "haz"_sv) >> fatal);
			}
			{
				// language + region
				{
					// static_assert(IETFLanguageTag::parse("haz-AF").has_value());
					// static_assert(IETFLanguageTag::parse("haz-AF")->operator std::string() == "haz-AF");

					const std::string string{"haz-AF"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
				}
				{
					const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number());
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
				}
			}
			{
				// language + script + region
				// static_assert(IETFLanguageTag::parse("haz-Arab-AF").has_value());
				// static_assert(IETFLanguageTag::parse("haz-Arab-AF")->operator std::string() == "haz-Arab-AF");

				const std::string string{"haz-Arab-AF"};
				const auto        result = IETFLanguageTag::parse(string);

				expect((result.has_value()) >> fatal);
				expect((result->operator std::string() == "haz-Arab-AF"_sv) >> fatal);
				expect((std::format("{}", *result) == "haz-Arab-AF"_sv) >> fatal);
			}
			{
				// Optional variant sub-tags, separated by hyphens, each composed of five to eight letters, or of four characters starting with a digit
				{
					const std::string string{"haz-polyton"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz"_sv) >> fatal);
				}
				{
					{
						const std::string string{"haz-AF-polyton"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
					{
						const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-polyton";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
				}
				{
					const std::string string{"haz-Arab-AF-polyton"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz-Arab-AF"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz-Arab-AF"_sv) >> fatal);
				}
			}
			{
				// Optional extension sub-tags, separated by hyphens, each composed of a single character, except the letter x, and a hyphen followed by one or more sub-tags of two to eight characters each, separated by hyphens
				{
					const std::string string{"haz-u-cu-usd"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz"_sv) >> fatal);
				}
				{
					{
						const std::string string{"haz-AF-u-cu-usd"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
					{
						const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-u-cu-usd";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
				}
				{
					const std::string string{"haz-Arab-AF-u-cu-usd"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz-Arab-AF"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz-Arab-AF"_sv) >> fatal);
				}
			}
			{
				// An optional private-use subtag, composed of the letter x and a hyphen followed by sub-tags of one to eight characters each, separated by hyphens
				{
					const std::string string{"haz-x-private"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz"_sv) >> fatal);
				}
				{
					{
						const std::string string{"haz-AF-x-private"};
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
					{
						const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-x-private";
						const auto        result = IETFLanguageTag::parse(string);

						expect((result.has_value()) >> fatal);
						expect((result->operator std::string() == "haz-AF"_sv) >> fatal);
						expect((std::format("{}", *result) == "haz-AF"_sv) >> fatal);
					}
				}
				{
					const std::string string{"haz-Arab-AF-x-private"};
					const auto        result = IETFLanguageTag::parse(string);

					expect((result.has_value()) >> fatal);
					expect((result->operator std::string() == "haz-Arab-AF"_sv) >> fatal);
					expect((std::format("{}", *result) == "haz-Arab-AF"_sv) >> fatal);
				}
			}
		};

		"shrink"_test = []
		{
			// aa-Latn-ET
			{
				// language
				const std::string string{"aa"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"aa-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"aa-Latn-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "aa"_sv) >> fatal);
			}
			// en-Latn-US
			{
				// language
				const std::string string{"en"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "en"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"en-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "en"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"en-Latn-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto shrink_result = result->shrink();
				expect((shrink_result.operator std::string() == "en"_sv) >> fatal);
			}
		};

		"expand"_test = []
		{
			// aa-Latn-ET
			{
				// language
				const std::string string{"aa"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto expand_result = result->expand();
				expect((expand_result.operator std::string() == "aa-Latn-ET"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"aa-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto expand_result = result->expand();
				expect((expand_result.operator std::string() == "aa-Latn-ET"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"aa-Latn-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto expand_result = result->expand();
				expect((expand_result.operator std::string() == "aa-Latn-ET"_sv) >> fatal);
			}
		};

		"variants"_test = []
		{
			// aa-Latn-ET
			{
				// language
				const std::string string{"aa"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->variants();
				expect((variants_result.size() == 1_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"aa-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->variants();
				expect((variants_result.size() == 2_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"aa-Latn-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "aa-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "aa"_sv) >> fatal);
			}
		};

		"canonical_variants"_test = []
		{
			// aa-Latn-ET
			{
				// language
				const std::string string{"aa"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 1_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"aa-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 2_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"aa-Latn-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "aa-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "aa"_sv) >> fatal);
			}
			// en-Latn-US
			{
				// language
				const std::string string{"en"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 1_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"en-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 2_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-US"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"en-Latn-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-Latn-US"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en-US"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "en-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "en"_sv) >> fatal);
			}
			// en-Latn-GB
			{
				// language + script + region
				const std::string string{"en-Latn-GB"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->canonical_variants();
				expect((variants_result.size() == 2_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-Latn-GB"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en-GB"_sv) >> fatal);
			}
		};

		"all_variants"_test = []
		{
			// aa-Latn-ET
			{
				// language
				const std::string string{"aa"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa"_sv) >> fatal);

				expect((variants_result[1].operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "aa-Latn"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"aa-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa"_sv) >> fatal);

				expect((variants_result[2].operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "aa-Latn"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"aa-Latn-ET"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "aa-Latn-ET"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "aa-ET"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "aa-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "aa"_sv) >> fatal);
			}
			// en-Latn-US
			{
				// language
				const std::string string{"en"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en"_sv) >> fatal);

				expect((variants_result[1].operator std::string() == "en-Latn-US"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "en-US"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "en-Latn"_sv) >> fatal);
			}
			{
				// language + region
				const std::string string{"en-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-US"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en"_sv) >> fatal);

				expect((variants_result[2].operator std::string() == "en-Latn-US"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "en-Latn"_sv) >> fatal);
			}
			{
				// language + script + region
				const std::string string{"en-Latn-US"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 4_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-Latn-US"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en-US"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "en-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "en"_sv) >> fatal);
			}
			// en-Latn-GB
			{
				// language + script + region
				const std::string string{"en-Latn-GB"};

				const auto result = IETFLanguageTag::parse(string);
				expect((result.has_value()) >> fatal);

				const auto variants_result = result->all_variants();
				expect((variants_result.size() == 6_ull) >> fatal);
				expect((variants_result[0].operator std::string() == "en-Latn-GB"_sv) >> fatal);
				expect((variants_result[1].operator std::string() == "en-GB"_sv) >> fatal);
				expect((variants_result[2].operator std::string() == "en-Latn"_sv) >> fatal);
				expect((variants_result[3].operator std::string() == "en"_sv) >> fatal);

				expect((variants_result[4].operator std::string() == "en-Latn-US"_sv) >> fatal);
				expect((variants_result[5].operator std::string() == "en-US"_sv) >> fatal);
			}
		};

		"static variants"_test = []
		{
			// Language order often used in the US:
			// - English with the US' local
			// - Fallback 1: English as spoken in Great-Britain.
			// - Fallback 2: French as spoken in the US.
			const std::vector languages{
					*IETFLanguageTag::parse("en-Latn-US"),
					*IETFLanguageTag::parse("en-Latn-GB"),
					*IETFLanguageTag::parse("fr-Latn-US")};

			const auto result = IETFLanguageTag::variants(languages);
			expect((result.size() == 12_ull) >> fatal);
			expect((result[0].operator std::string() == "en-Latn-US"_sv) >> fatal);
			expect((result[1].operator std::string() == "en-US"_sv) >> fatal);
			expect((result[2].operator std::string() == "en-Latn"_sv) >> fatal);
			expect((result[3].operator std::string() == "en"_sv) >> fatal);
			expect((result[4].operator std::string() == "en-Latn-GB"_sv) >> fatal);
			expect((result[5].operator std::string() == "en-GB"_sv) >> fatal);
			expect((result[6].operator std::string() == "fr-Latn-US"_sv) >> fatal);
			expect((result[7].operator std::string() == "fr-US"_sv) >> fatal);
			expect((result[8].operator std::string() == "fr-Latn"_sv) >> fatal);
			expect((result[9].operator std::string() == "fr"_sv) >> fatal);
			expect((result[10].operator std::string() == "fr-Latn-FR"_sv) >> fatal);
			expect((result[11].operator std::string() == "fr-FR"_sv) >> fatal);
		};
	};
}// namespace
