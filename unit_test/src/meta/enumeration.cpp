// meta::enumeration
#include <meta/meta.hpp>
// functional::enumeration
#include <functional/functional.hpp>

using namespace gal::prometheus;

namespace
{
	enum FreeEnum0 : std::uint8_t
	{
		FE0_E1 = 0,
		FE0_E2 = 1,
		FE0_E3 = 2,
		FE0_E4 = 3,
	};

	enum FreeEnum1: std::uint8_t
	{
		FE1_E1 = 1,
		FE1_E2 = 2,
		FE1_E3 = 3,
		FE1_E4 = 4,
	};

	enum class ScopedEnum0: std::uint8_t
	{
		E1 = 0,
		E2 = 1,
		E3 = 2,
		E4 = 3,
	};

	enum class ScopedEnum1: std::uint8_t
	{
		E1 = 1,
		E2 = 2,
		E3 = 3,
		E4 = 4,
	};

	enum FreeFlag0: std::uint8_t
	{
		FF0_F0 = 0b0000,
		FF0_F1 = 0b0001,
		FF0_F2 = 0b0010,
		FF0_F3 = 0b0100,
		FF0_F4 = 0b1000,

		FF0_F5 = FF0_F1 | FF0_F2,
		FF0_F6 = FF0_F3 | FF0_F4,
	};

	enum FreeFlag1: std::uint8_t
	{
		FF1_F1 = 0b0001,
		FF1_F2 = 0b0010,
		FF1_F3 = 0b0100,
		FF1_F4 = 0b1000,

		FF1_F5 = FF1_F1 | FF1_F2,
		FF1_F6 = FF1_F3 | FF1_F4,
	};

	enum class ScopedFlag0: std::uint8_t
	{
		F0 = 0b0000,
		F1 = 0b0001,
		F2 = 0b0010,
		F3 = 0b0100,
		F4 = 0b1000,

		F5 = F1 | F2,
		F6 = F3 | F4,

		PROMETHEUS_MAGIC_ENUM_FLAG [[maybe_unused]]
	};

	enum class ScopedFlag1: std::uint8_t
	{
		F1 = 0b0001,
		F2 = 0b0010,
		F3 = 0b0100,
		F4 = 0b1000,

		F5 = F1 | F2,
		F6 = F3 | F4,

		PROMETHEUS_MAGIC_ENUM_FLAG [[maybe_unused]]
	};

	// ==========================================================================
	// min/max

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
	
	// ==========================================================================
	// name_of
	
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E1) == "FE0_E1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E2) == "FE0_E2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E3) == "FE0_E3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE0_E4) == "FE0_E4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeEnum0>(FE0_E4 + 1)) == meta::enum_name_not_found);
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E1) == "FE1_E1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E2) == "FE1_E2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E3) == "FE1_E3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FE1_E4) == "FE1_E4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeEnum1>(FE1_E4 + 1)) == meta::enum_name_not_found);
	
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E1) == "ScopedEnum0::E1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E2) == "ScopedEnum0::E2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E3) == "ScopedEnum0::E3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum0::E4) == "ScopedEnum0::E4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedEnum0>(std::to_underlying(ScopedEnum0::E4) + 1)) == meta::enum_name_not_found);
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E1) == "ScopedEnum1::E1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E2) == "ScopedEnum1::E2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E3) == "ScopedEnum1::E3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedEnum1::E4) == "ScopedEnum1::E4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedEnum1>(std::to_underlying(ScopedEnum1::E4) + 1)) == meta::enum_name_not_found);
	
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
	
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F1) == "FF1_F1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F2) == "FF1_F2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F3) == "FF1_F3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F4) == "FF1_F4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F5) == "FF1_F5");
	static_assert(meta::name_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F1 | FF1_F2) == "FF1_F5");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F6) == "FF1_F6");
	static_assert(meta::name_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>(FF1_F3 | FF1_F4) == "FF1_F6");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<FreeFlag1>(FF1_F6 + 1)) == meta::enum_name_not_found);
	
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F0) == "ScopedFlag0::F0");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F1) == "ScopedFlag0::F1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F2) == "ScopedFlag0::F2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F3) == "ScopedFlag0::F3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F4) == "ScopedFlag0::F4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F5) == "ScopedFlag0::F5");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F1 | ScopedFlag0::F2) == "ScopedFlag0::F5");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F6) == "ScopedFlag0::F6");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag0::F3 | ScopedFlag0::F4) == "ScopedFlag0::F6");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedFlag0>(std::to_underlying(ScopedFlag0::F6) + 1)) == meta::enum_name_not_found);
	
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F1) == "ScopedFlag1::F1");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F2) == "ScopedFlag1::F2");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F3) == "ScopedFlag1::F3");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F4) == "ScopedFlag1::F4");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F5) == "ScopedFlag1::F5");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F1 | ScopedFlag1::F2) == "ScopedFlag1::F5");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F6) == "ScopedFlag1::F6");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F3 | ScopedFlag1::F4) == "ScopedFlag1::F6");
	static_assert(meta::name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(static_cast<ScopedFlag1>(std::to_underlying(ScopedFlag1::F6) + 1)) == meta::enum_name_not_found);
	
	// ==========================================================================
	// full_name_of
	
	template<meta::EnumNamePolicy Policy>
	[[nodiscard]] constexpr auto test_full_name_of(const auto e, const auto expected, const auto split) noexcept -> bool
	{
		return meta::full_name_of<Policy>(e, split) == expected;
	}
	
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF0_F5, "FF0_F1-FF0_F2", "-"));
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF0_F6, "FF0_F3-FF0_F4", "-"));
	
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF1_F5, "FF1_F1/FF1_F2", "/"));
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(FF1_F6, "FF1_F3/FF1_F4", "/"));
	
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(ScopedFlag0::F5, "F1-F2", "-"));
	static_assert(test_full_name_of<meta::EnumNamePolicy::VALUE_ONLY>(ScopedFlag0::F6, "F3-F4", "-"));
	
	static_assert(test_full_name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F5, "ScopedFlag1::F1/ScopedFlag1::F2", "/"));
	static_assert(test_full_name_of<meta::EnumNamePolicy::WITH_SCOPED_NAME>(ScopedFlag1::F6, "ScopedFlag1::F3/ScopedFlag1::F4", "/"));

	// ==========================================================================
	// value_of

	static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E1") == FE0_E1);
	static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E2") == FE0_E2);
	static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E3") == FE0_E3);
	static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E4") == FE0_E4);
	static_assert(meta::value_of<FreeEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE0_E5", static_cast<FreeEnum0>(42)) == 42);

	static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E1") == FE1_E1);
	static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E2") == FE1_E2);
	static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E3") == FE1_E3);
	static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E4") == FE1_E4);
	static_assert(meta::value_of<FreeEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FE1_E5", static_cast<FreeEnum1>(42)) == 42);

	static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E1") == ScopedEnum0::E1);
	static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E2") == ScopedEnum0::E2);
	static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E3") == ScopedEnum0::E3);
	static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E4") == ScopedEnum0::E4);
	static_assert(meta::value_of<ScopedEnum0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum0::E5") == static_cast<ScopedEnum0>(0));

	static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E1") == ScopedEnum1::E1);
	static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E2") == ScopedEnum1::E2);
	static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E3") == ScopedEnum1::E3);
	static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E4") == ScopedEnum1::E4);
	static_assert(meta::value_of<ScopedEnum1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedEnum1::E5") == static_cast<ScopedEnum1>(0));

	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F0") == FF0_F0);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F1") == FF0_F1);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F2") == FF0_F2);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F3") == FF0_F3);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F4") == FF0_F4);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F5") == FF0_F5);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F1|FF0_F2") == FF0_F5);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F6") == FF0_F6);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F3|FF0_F4") == FF0_F6);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("FF0_F3|FF0_F1337", static_cast<FreeFlag0>(42)) == 42);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("FF0_F3|FF0_F1337") == FF0_F3);
	static_assert(meta::value_of<FreeFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF0_F7", static_cast<FreeFlag0>(42)) == 42);

	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F1") == FF1_F1);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F2") == FF1_F2);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F3") == FF1_F3);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F4") == FF1_F4);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F5") == FF1_F5);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F1|FF1_F2") == FF1_F5);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F6") == FF1_F6);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F3|FF1_F4") == FF1_F6);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("FF1_F3|FF1_F1337", static_cast<FreeFlag1>(42)) == 42);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("FF1_F3|FF1_F1337") == FF1_F3);
	static_assert(meta::value_of<FreeFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("FF1_F7", static_cast<FreeFlag1>(42)) == 42);

	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F0", "+") == ScopedFlag0::F0);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F1", "+") == ScopedFlag0::F1);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F2", "+") == ScopedFlag0::F2);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F3", "+") == ScopedFlag0::F3);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F4", "+") == ScopedFlag0::F4);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F5", "+") == ScopedFlag0::F5);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F1+ScopedFlag0::F2", "+") == ScopedFlag0::F5);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F6", "+") == ScopedFlag0::F6);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F3+ScopedFlag0::F4", "+") == ScopedFlag0::F6);
	static_assert(
		meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("ScopedFlag0::F3+ScopedFlag0::F1337", "+", static_cast<ScopedFlag0>(42)) ==
		static_cast<ScopedFlag0>(42)
	);
	static_assert(meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("ScopedFlag0::F3+ScopedFlag0::F1337", "+") == ScopedFlag0::F3);
	static_assert(
		meta::value_of<ScopedFlag0, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag0::F7", "+", static_cast<ScopedFlag0>(42)) ==
		static_cast<ScopedFlag0>(42)
	);

	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F1", "-") == ScopedFlag1::F1);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F2", "-") == ScopedFlag1::F2);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F3", "-") == ScopedFlag1::F3);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F4", "-") == ScopedFlag1::F4);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F5", "-") == ScopedFlag1::F5);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F1-ScopedFlag1::F2", "-") == ScopedFlag1::F5);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F6", "-") == ScopedFlag1::F6);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F3-ScopedFlag1::F4", "-") == ScopedFlag1::F6);
	static_assert(
		meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, true>("ScopedFlag1::F3-ScopedFlag1::F1337", "-", static_cast<ScopedFlag1>(42)) ==
		static_cast<ScopedFlag1>(42)
	);
	static_assert(meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME, false>("ScopedFlag1::F3-ScopedFlag1::F1337", "-") == ScopedFlag1::F3);
	static_assert(
		meta::value_of<ScopedFlag1, meta::EnumNamePolicy::WITH_SCOPED_NAME>("ScopedFlag1::F7", "-", static_cast<ScopedFlag1>(42)) ==
		static_cast<ScopedFlag1>(42)
	);
}
