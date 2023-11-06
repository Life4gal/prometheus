#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.i18n;

namespace
{
	using namespace gal::prometheus;
	using namespace i18n;

	GAL_PROMETHEUS_NO_DESTROY test::suite<"i18n.ietf_language_tag"> _ = []
	{
		using namespace test;

		ignore_pass / "parse"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};
					const auto		  result = IETFLanguageTag::parse(string);

					expect(result.has_value()) << fatal;
					expect(result->operator std::string() == "aa"_s) << fatal;
					expect(std::format("{}", *result) == "aa"_s) << fatal;
				};

				"language+region"_test = []
				{
					{
						const std::string string{"aa-ET"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa-ET"_s) << fatal;
						expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
					}
					{
						const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number());
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa-ET"_s) << fatal;
						expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
					}
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};
					const auto		  result = IETFLanguageTag::parse(string);

					expect(result.has_value()) << fatal;
					expect(result->operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(std::format("{}", *result) == "aa-Latn-ET"_s) << fatal;
				};

				"Optional variant sub-tags, separated by hyphens, each composed of five to eight letters, or of four characters starting with a digit"_test = []
				{
					{
						const std::string string{"aa-polyton"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa"_s) << fatal;
						expect(std::format("{}", *result) == "aa"_s) << fatal;
					}
					{
						{
							const std::string string{"aa-ET-polyton"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
						{
							const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-polyton";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
					}
					{
						const std::string string{"aa-Latn-ET-polyton"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa-Latn-ET"_s) << fatal;
						expect(std::format("{}", *result) == "aa-Latn-ET"_s) << fatal;
					}
				};

				"Optional extension sub-tags, separated by hyphens, each composed of a single character, except the letter x, and a hyphen followed by one or more sub-tags of two to eight characters each, separated by hyphens"_test = []
				{
					{
						const std::string string{"aa-u-cu-usd"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa"_s) << fatal;
						expect(std::format("{}", *result) == "aa"_s) << fatal;
					}
					{
						{
							const std::string string{"aa-ET-u-cu-usd"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
						{
							const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-u-cu-usd";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
					}
					{
						const std::string string{"aa-Latn-ET-u-cu-usd"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa-Latn-ET"_s) << fatal;
						expect(std::format("{}", *result) == "aa-Latn-ET"_s) << fatal;
					}
				};

				"An optional private-use subtag, composed of the letter x and a hyphen followed by sub-tags of one to eight characters each, separated by hyphens"_test = []
				{
					{
						const std::string string{"aa-x-private"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa"_s) << fatal;
						expect(std::format("{}", *result) == "aa"_s) << fatal;
					}
					{
						{
							const std::string string{"aa-ET-x-private"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
						{
							const std::string string = "aa-" + std::format("{:0>3}", ISO3166::parse("ET")->number()) + "-x-private";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "aa-ET"_s) << fatal;
							expect(std::format("{}", *result) == "aa-ET"_s) << fatal;
						}
					}
					{
						const std::string string{"aa-Latn-ET-x-private"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "aa-Latn-ET"_s) << fatal;
						expect(std::format("{}", *result) == "aa-Latn-ET"_s) << fatal;
					}
				};
			};

			"haz-Arab-AF"_test = []
			{
				"language"_test = []
				{
					const std::string string{"haz"};
					const auto		  result = IETFLanguageTag::parse(string);

					expect(result.has_value()) << fatal;
					expect(result->operator std::string() == "haz"_s) << fatal;
					expect(std::format("{}", *result) == "haz"_s) << fatal;
				};

				"language+region"_test = []
				{
					{
						const std::string string{"haz-AF"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz-AF"_s) << fatal;
						expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
					}
					{
						const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number());
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz-AF"_s) << fatal;
						expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
					}
				};

				"language+script+region"_test = []
				{
					const std::string string{"haz-Arab-AF"};
					const auto		  result = IETFLanguageTag::parse(string);

					expect(result.has_value()) << fatal;
					expect(result->operator std::string() == "haz-Arab-AF"_s) << fatal;
					expect(std::format("{}", *result) == "haz-Arab-AF"_s) << fatal;
				};

				"Optional variant sub-tags, separated by hyphens, each composed of five to eight letters, or of four characters starting with a digit"_test = []
				{
					{
						const std::string string{"haz-polyton"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz"_s) << fatal;
						expect(std::format("{}", *result) == "haz"_s) << fatal;
					}
					{
						{
							const std::string string{"haz-AF-polyton"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
						{
							const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-polyton";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
					}
					{
						const std::string string{"haz-Arab-AF-polyton"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz-Arab-AF"_s) << fatal;
						expect(std::format("{}", *result) == "haz-Arab-AF"_s) << fatal;
					}
				};

				"Optional extension sub-tags, separated by hyphens, each composed of a single character, except the letter x, and a hyphen followed by one or more sub-tags of two to eight characters each, separated by hyphens"_test = []
				{
					{
						const std::string string{"haz-u-cu-usd"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz"_s) << fatal;
						expect(std::format("{}", *result) == "haz"_s) << fatal;
					}
					{
						{
							const std::string string{"haz-AF-u-cu-usd"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
						{
							const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-u-cu-usd";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
					}
					{
						const std::string string{"haz-Arab-AF-u-cu-usd"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz-Arab-AF"_s) << fatal;
						expect(std::format("{}", *result) == "haz-Arab-AF"_s) << fatal;
					}
				};

				"An optional private-use subtag, composed of the letter x and a hyphen followed by sub-tags of one to eight characters each, separated by hyphens"_test = []
				{
					{
						const std::string string{"haz-x-private"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz"_s) << fatal;
						expect(std::format("{}", *result) == "haz"_s) << fatal;
					}
					{
						{
							const std::string string{"haz-AF-x-private"};
							const auto result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
						{
							const std::string string = "haz-" + std::format("{:0>3}", ISO3166::parse("AF")->number()) + "-x-private";
							const auto		  result = IETFLanguageTag::parse(string);

							expect(result.has_value()) << fatal;
							expect(result->operator std::string() == "haz-AF"_s) << fatal;
							expect(std::format("{}", *result) == "haz-AF"_s) << fatal;
						}
					}
					{
						const std::string string{"haz-Arab-AF-x-private"};
						const auto		  result = IETFLanguageTag::parse(string);

						expect(result.has_value()) << fatal;
						expect(result->operator std::string() == "haz-Arab-AF"_s) << fatal;
						expect(std::format("{}", *result) == "haz-Arab-AF"_s) << fatal;
					}
				};
			};
		};

		ignore_pass / "shrink"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "aa"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"aa-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "aa"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "aa"_s) << fatal;
				};
			};

			"en-Latn-US"_test = []
			{
				"language"_test = []
				{
					const std::string string{"en"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "en"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"en-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "en"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"en-Latn-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto shrink_result = result->shrink();
					expect(shrink_result.operator std::string() == "en"_s) << fatal;
				};
			};
		};

		ignore_pass / "expand"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto expand_result = result->expand();
					expect(expand_result.operator std::string() == "aa-Latn-ET"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"aa-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto expand_result = result->expand();
					expect(expand_result.operator std::string() == "aa-Latn-ET"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto expand_result = result->expand();
					expect(expand_result.operator std::string() == "aa-Latn-ET"_s) << fatal;
				};
			};
		};

		ignore_pass / "variants"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->variants();
					expect(variants_result.size() == 1_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"aa-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->variants();
					expect(variants_result.size() == 2_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[2].operator std::string() == "aa-Latn"_s) << fatal;
					expect(variants_result[3].operator std::string() == "aa"_s) << fatal;
				};
			};
		};

		ignore_pass / "canonical_variants"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 1_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"aa-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 2_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[2].operator std::string() == "aa-Latn"_s) << fatal;
					expect(variants_result[3].operator std::string() == "aa"_s) << fatal;
				};
			};

			"en-Latn-US"_test = []
			{
				"language"_test = []
				{
					const std::string string{"en"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 1_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en"_s) << fatal;
				};

				"langugae+region"_test = []
				{
					const std::string string{"en-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 2_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en-US"_s) << fatal;
					expect(variants_result[1].operator std::string() == "en"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"en-Latn-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->canonical_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en-Latn-US"_s) << fatal;
					expect(variants_result[1].operator std::string() == "en-US"_s) << fatal;
					expect(variants_result[2].operator std::string() == "en-Latn"_s) << fatal;
					expect(variants_result[3].operator std::string() == "en"_s) << fatal;
				};
			};

			"en-Latn-GB"_test = []
			{
				const std::string string{"en-Latn-GB"};

				const auto		  result = IETFLanguageTag::parse(string);
				expect(result.has_value()) << fatal;

				const auto variants_result = result->canonical_variants();
				expect(variants_result.size() == 2_auto) << fatal;
				expect(variants_result[0].operator std::string() == "en-Latn-GB"_s) << fatal;
				expect(variants_result[1].operator std::string() == "en-GB"_s) << fatal;
			};
		};

		ignore_pass / "all_variants"_test = []
		{
			"aa-Latn-ET"_test = []
			{
				"language"_test = []
				{
					const std::string string{"aa"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa"_s) << fatal;

					expect(variants_result[1].operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(variants_result[2].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[3].operator std::string() == "aa-Latn"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"aa-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa"_s) << fatal;

					expect(variants_result[2].operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(variants_result[3].operator std::string() == "aa-Latn"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"aa-Latn-ET"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "aa-Latn-ET"_s) << fatal;
					expect(variants_result[1].operator std::string() == "aa-ET"_s) << fatal;
					expect(variants_result[2].operator std::string() == "aa-Latn"_s) << fatal;
					expect(variants_result[3].operator std::string() == "aa"_s) << fatal;
				};
			};

			"en-Latn-US"_test = []
			{
				"language"_test = []
				{
					const std::string string{"en"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en"_s) << fatal;

					expect(variants_result[1].operator std::string() == "en-Latn-US"_s) << fatal;
					expect(variants_result[2].operator std::string() == "en-US"_s) << fatal;
					expect(variants_result[3].operator std::string() == "en-Latn"_s) << fatal;
				};

				"language+region"_test = []
				{
					const std::string string{"en-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en-US"_s) << fatal;
					expect(variants_result[1].operator std::string() == "en"_s) << fatal;

					expect(variants_result[2].operator std::string() == "en-Latn-US"_s) << fatal;
					expect(variants_result[3].operator std::string() == "en-Latn"_s) << fatal;
				};

				"language+script+region"_test = []
				{
					const std::string string{"en-Latn-US"};

					const auto		  result = IETFLanguageTag::parse(string);
					expect(result.has_value()) << fatal;

					const auto variants_result = result->all_variants();
					expect(variants_result.size() == 4_auto) << fatal;
					expect(variants_result[0].operator std::string() == "en-Latn-US"_s) << fatal;
					expect(variants_result[1].operator std::string() == "en-US"_s) << fatal;
					expect(variants_result[2].operator std::string() == "en-Latn"_s) << fatal;
					expect(variants_result[3].operator std::string() == "en"_s) << fatal;
				};
			};

			"en-Latn-GB"_test = []
			{
				const std::string string{"en-Latn-GB"};

				const auto		  result = IETFLanguageTag::parse(string);
				expect(result.has_value()) << fatal;

				const auto variants_result = result->all_variants();
				expect(variants_result.size() == 6_auto) << fatal;
				expect(variants_result[0].operator std::string() == "en-Latn-GB"_s) << fatal;
				expect(variants_result[1].operator std::string() == "en-GB"_s) << fatal;
				expect(variants_result[2].operator std::string() == "en-Latn"_s) << fatal;
				expect(variants_result[3].operator std::string() == "en"_s) << fatal;

				expect(variants_result[4].operator std::string() == "en-Latn-US"_s) << fatal;
				expect(variants_result[5].operator std::string() == "en-US"_s) << fatal;
			};
		};

		ignore_pass / "static variants"_test = []
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
			expect(result.size() == 12_auto) << fatal;
			expect(result[0].operator std::string() == "en-Latn-US"_s) << fatal;
			expect(result[1].operator std::string() == "en-US"_s) << fatal;
			expect(result[2].operator std::string() == "en-Latn"_s) << fatal;
			expect(result[3].operator std::string() == "en"_s) << fatal;
			expect(result[4].operator std::string() == "en-Latn-GB"_s) << fatal;
			expect(result[5].operator std::string() == "en-GB"_s) << fatal;
			expect(result[6].operator std::string() == "fr-Latn-US"_s) << fatal;
			expect(result[7].operator std::string() == "fr-US"_s) << fatal;
			expect(result[8].operator std::string() == "fr-Latn"_s) << fatal;
			expect(result[9].operator std::string() == "fr"_s) << fatal;
			expect(result[10].operator std::string() == "fr-Latn-FR"_s) << fatal;
			expect(result[11].operator std::string() == "fr-FR"_s) << fatal;
		};
	};
}// namespace
