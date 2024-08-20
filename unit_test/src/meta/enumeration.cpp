#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_USE_MODULE
import std;
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

using namespace gal::prometheus;

namespace
{
	enum FreeEnum0
	{
		FE0_E1 = 0,
		FE0_E2 = 1,
		FE0_E3 = 2,
		FE0_E4 = 3,
	};

	enum FreeEnum1
	{
		FE1_E1 = 1,
		FE1_E2 = 2,
		FE1_E3 = 3,
		FE1_E4 = 4,
	};

	enum class ScopedEnum0
	{
		E1 = 0,
		E2 = 1,
		E3 = 2,
		E4 = 3,
	};

	enum class ScopedEnum1
	{
		E1 = 1,
		E2 = 2,
		E3 = 3,
		E4 = 4,
	};

	enum FreeFlag0
	{
		FF0_F0 = 0b0000,
		FF0_F1 = 0b0001,
		FF0_F2 = 0b0010,
		FF0_F3 = 0b0100,
		FF0_F4 = 0b1000,

		FF0_F5 = FF0_F1 | FF0_F2,
		FF0_F6 = FF0_F3 | FF0_F4,
	};

	enum FreeFlag1
	{
		FF1_F1 = 0b0001,
		FF1_F2 = 0b0010,
		FF1_F3 = 0b0100,
		FF1_F4 = 0b1000,

		FF1_F5 = FF1_F1 | FF1_F2,
		FF1_F6 = FF1_F3 | FF1_F4,
	};

	enum class ScopedFlag0
	{
		F0 = 0b0000,
		F1 = 0b0001,
		F2 = 0b0010,
		F3 = 0b0100,
		F4 = 0b1000,

		F5 = F1 | F2,
		F6 = F3 | F4,
	};

	enum class ScopedFlag1
	{
		F1 = 0b0001,
		F2 = 0b0010,
		F3 = 0b0100,
		F4 = 0b1000,

		F5 = F1 | F2,
		F6 = F3 | F4,
	};

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"meta.enumeration"> _ = []
	{
		using namespace unit_test;
		using namespace literals;

		"min/max"_test = []
		{
			static_assert(meta::min_value_of<FreeEnum0>() == FE0_E1);
			static_assert(meta::max_value_of<FreeEnum0>() == FE0_E4);

			static_assert(meta::min_value_of<FreeEnum1>() == FE1_E1);
			static_assert(meta::max_value_of<FreeEnum1>() == FE1_E4);

			static_assert(meta::min_value_of<ScopedEnum0>() == std::to_underlying(ScopedEnum0::E1));
			static_assert(meta::max_value_of<ScopedEnum0>() == std::to_underlying(ScopedEnum0::E4));

			static_assert(meta::min_value_of<ScopedEnum1>() == std::to_underlying(ScopedEnum1::E1));
			static_assert(meta::max_value_of<ScopedEnum1>() == std::to_underlying(ScopedEnum1::E4));

			static_assert(meta::min_value_of<FreeFlag0>() == FF0_F0);
			static_assert(meta::max_value_of<FreeFlag0>() == FF0_F6);

			static_assert(meta::min_value_of<FreeFlag1>() == FF1_F1);
			static_assert(meta::max_value_of<FreeFlag1>() == FF1_F6);

			static_assert(meta::min_value_of<ScopedFlag0>() == std::to_underlying(ScopedFlag0::F0));
			static_assert(meta::max_value_of<ScopedFlag0>() == std::to_underlying(ScopedFlag0::F6));

			static_assert(meta::min_value_of<ScopedFlag1>() == std::to_underlying(ScopedFlag1::F1));
			static_assert(meta::max_value_of<ScopedFlag1>() == std::to_underlying(ScopedFlag1::F6));
		};

		"name_of"_test = []
		{
			"FreeEnum0"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E1) == "FE0_E1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E2) == "FE0_E2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E3) == "FE0_E3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E4) == "FE0_E4");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeEnum0>(FE0_E4 + 1)) == meta::enum_name_not_found);
			};

			"FreeEnum1"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E1) == "FE1_E1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E2) == "FE1_E2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E3) == "FE1_E3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E4) == "FE1_E4");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeEnum1>(FE1_E4 + 1)) == meta::enum_name_not_found);
			};

			"ScopedEnum0"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E1) == "ScopedEnum0::E1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E2) == "ScopedEnum0::E2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E3) == "ScopedEnum0::E3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E4) == "ScopedEnum0::E4");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedEnum0>(std::to_underlying(ScopedEnum0::E4) + 1)) == meta::enum_name_not_found);
			};

			"ScopedEnum1"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E1) == "ScopedEnum1::E1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E2) == "ScopedEnum1::E2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E3) == "ScopedEnum1::E3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E4) == "ScopedEnum1::E4");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedEnum1>(std::to_underlying(ScopedEnum1::E4) + 1)) == meta::enum_name_not_found);
			};

			"FreeFlag0"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F0) == "FF0_F0");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F1) == "FF0_F1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F2) == "FF0_F2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F3) == "FF0_F3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F4) == "FF0_F4");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F5) == "FF0_F5");
				static_assert(meta::name_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F1 | FF0_F2) == "FF0_F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F6) == "FF0_F6");
				static_assert(meta::name_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF0_F3 | FF0_F4) == "FF0_F6");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeFlag0>(FF0_F6 + 1)) == meta::enum_name_not_found);
			};

			"FreeFlag1"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F1) == "FF1_F1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F2) == "FF1_F2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F3) == "FF1_F3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F4) == "FF1_F4");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F5) == "FF1_F5");
				static_assert(meta::name_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F1 | FF1_F2) == "FF1_F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F6) == "FF1_F6");
				static_assert(meta::name_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F3 | FF1_F4) == "FF1_F6");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeFlag1>(FF1_F6 + 1)) == meta::enum_name_not_found);
			};

			"ScopedFlag0"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F0) == "ScopedFlag0::F0");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F1) == "ScopedFlag0::F1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F2) == "ScopedFlag0::F2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F3) == "ScopedFlag0::F3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F4) == "ScopedFlag0::F4");

				using functional::operators::operator|;

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F5) == "ScopedFlag0::F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F1 | ScopedFlag0::F2) == "ScopedFlag0::F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F6) == "ScopedFlag0::F6");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F3 | ScopedFlag0::F4) == "ScopedFlag0::F6");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedFlag0>(std::to_underlying(ScopedFlag0::F6) + 1)) == meta::enum_name_not_found);
			};

			"ScopedFlag1"_test = []
			{
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F1) == "ScopedFlag1::F1");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F2) == "ScopedFlag1::F2");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F3) == "ScopedFlag1::F3");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F4) == "ScopedFlag1::F4");

				using functional::operators::operator|;

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F5) == "ScopedFlag1::F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F1 | ScopedFlag1::F2) == "ScopedFlag1::F5");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F6) == "ScopedFlag1::F6");
				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F3 | ScopedFlag1::F4) == "ScopedFlag1::F6");

				static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedFlag1>(std::to_underlying(ScopedFlag1::F6) + 1)) == meta::enum_name_not_found);
			};

			"full_name_of"_test = []
			{
				"FreeFlag0"_test = []
				{
					using namespace unit_test::operators;

					constexpr std::string_view split{"-"};

					expect((meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF0_F5, split) == "FF0_F1-FF0_F2") == "FF0_F5 == FF0_F1-FF0_F2"_b);
					expect((meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF0_F6, split) == "FF0_F3-FF0_F4") == "FF0_F6 == FF0_F3-FF0_F4"_b);
				};

				"FreeFlag1"_test = []
				{
					using namespace unit_test::operators;

					constexpr std::string_view split{"/"};

					expect((meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF1_F5, split) == "FF1_F1/FF1_F2") == "FF1_F5 == FF1_F1/FF1_F2"_b);
					expect((meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF1_F6, split) == "FF1_F3/FF1_F4") == "FF1_F6 == FF1_F3/FF1_F4"_b);
				};

				"ScopedFlag0"_test = []
				{
					using namespace unit_test::operators;

					constexpr std::string_view split{"-"};

					expect(
						(meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(ScopedFlag0::F5, split) == "F1-F2") ==
						"ScopedFlag0::F5 == ScopedFlag0::F1-ScopedFlag0::F2"_b
					);
					expect(
						(meta::full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(ScopedFlag0::F6, split) == "F3-F4") ==
						"ScopedFlag0::F6 == ScopedFlag0::F3-ScopedFlag0::F4"_b
					);
				};

				"ScopedFlag1"_test = []
				{
					using namespace unit_test::operators;

					constexpr std::string_view split{"/"};

					expect(
						(meta::full_name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F5, split) == "ScopedFlag1::F1/ScopedFlag1::F2") ==
						"ScopedFlag1::F5 == ScopedFlag1::F1/ScopedFlag1::F2"_b
					);
					expect(
						(meta::full_name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F6, split) == "ScopedFlag1::F3/ScopedFlag1::F4") ==
						"ScopedFlag1::F6 == ScopedFlag1::F3/ScopedFlag1::F4"_b
					);
				};
			};
		};

		"value_of"_test = []
		{
			"FreeEnum0"_test = []
			{
				static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E1").value() == FE0_E1);
				static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E2").value() == FE0_E2);
				static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E3").value() == FE0_E3);
				static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E4").value() == FE0_E4);
				static_assert(not meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E5").has_value());
			};

			"FreeEnum1"_test = []
			{
				static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E1").value() == FE1_E1);
				static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E2").value() == FE1_E2);
				static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E3").value() == FE1_E3);
				static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E4").value() == FE1_E4);
				static_assert(not meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E5").has_value());
			};

			"ScopedEnum0"_test = []
			{
				static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E1").value() == ScopedEnum0::E1);
				static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E2").value() == ScopedEnum0::E2);
				static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E3").value() == ScopedEnum0::E3);
				static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E4").value() == ScopedEnum0::E4);
				static_assert(not meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E5").has_value());
			};

			"ScopedEnum1"_test = []
			{
				static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E1").value() == ScopedEnum1::E1);
				static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E2").value() == ScopedEnum1::E2);
				static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E3").value() == ScopedEnum1::E3);
				static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E4").value() == ScopedEnum1::E4);
				static_assert(not meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E5").has_value());
			};

			"FreeFlag0"_test = []
			{
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F0").value() == FF0_F0);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F1").value() == FF0_F1);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F2").value() == FF0_F2);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F3").value() == FF0_F3);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F4").value() == FF0_F4);

				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F5").value() == FF0_F5);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F1|FF0_F2").value() == FF0_F5);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F6").value() == FF0_F6);
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F3|FF0_F4").value() == FF0_F6);

				static_assert(not meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("FF0_F3|FF0_F1337").has_value());
				static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("FF0_F3|FF0_F1337").value() == FF0_F3);

				static_assert(not meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F7").has_value());
			};

			"FreeFlag1"_test = []
			{
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F1").value() == FF1_F1);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F2").value() == FF1_F2);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F3").value() == FF1_F3);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F4").value() == FF1_F4);

				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F5").value() == FF1_F5);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F1|FF1_F2").value() == FF1_F5);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F6").value() == FF1_F6);
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F3|FF1_F4").value() == FF1_F6);

				static_assert(not meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("FF1_F3|FF1_F1337").has_value());
				static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("FF1_F3|FF1_F1337").value() == FF1_F3);

				static_assert(not meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F7").has_value());
			};

			"ScopedFlag0"_test = []
			{
				constexpr std::string_view split{"+"};

				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F0", split).value() == ScopedFlag0::F0);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F1", split).value() == ScopedFlag0::F1);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F2", split).value() == ScopedFlag0::F2);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F3", split).value() == ScopedFlag0::F3);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F4", split).value() == ScopedFlag0::F4);

				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F5", split).value() == ScopedFlag0::F5);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F1+ScopedFlag0::F2", split).value() == ScopedFlag0::F5);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F6", split).value() == ScopedFlag0::F6);
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F3+ScopedFlag0::F4", split).value() == ScopedFlag0::F6);

				static_assert(not meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("ScopedFlag0::F3+ScopedFlag0::F1337", split).has_value());
				static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("ScopedFlag0::F3+ScopedFlag0::F1337", split).value() == ScopedFlag0::F3);

				static_assert(not meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F7", split).has_value());
			};

			"ScopedFlag1"_test = []
			{
				constexpr std::string_view split{"-"};

				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F1", split).value() == ScopedFlag1::F1);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F2", split).value() == ScopedFlag1::F2);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F3", split).value() == ScopedFlag1::F3);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F4", split).value() == ScopedFlag1::F4);

				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F5", split).value() == ScopedFlag1::F5);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F1-ScopedFlag1::F2", split).value() == ScopedFlag1::F5);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F6", split).value() == ScopedFlag1::F6);
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F3-ScopedFlag1::F4", split).value() == ScopedFlag1::F6);

				static_assert(not meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("ScopedFlag1::F3-ScopedFlag1::F1337", split).has_value());
				static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("ScopedFlag1::F3-ScopedFlag1::F1337", split).value() == ScopedFlag1::F3);

				static_assert(not meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F7", split).has_value());
			};
		};
	};
}
