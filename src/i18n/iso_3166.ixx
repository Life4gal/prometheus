// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.i18n:iso_3166;

import std;
import gal.prometheus.infrastructure;

namespace gal::prometheus::i18n
{
	export
	{
		class IETFLanguageTag;

		/**
		 * @brief ISO-3166 country code.
		 */
		// ReSharper disable once CppInconsistentNaming
		class ISO3166 final
		{
			friend IETFLanguageTag;

		public:
			using value_type = std::uint16_t;

			using element_type = char;

			using code_info_code2_type = infrastructure::fixed_string<2>;
			using code_info_code3_type = infrastructure::fixed_string<3>;

			struct code_info
			{
				code_info_code2_type code2;
				code_info_code3_type code3;
				value_type           number;
			};

			// 001 ~ 999
			constexpr static auto code_info_max_size = 1000;

			constexpr static auto parse(std::basic_string_view<element_type> string) noexcept -> std::optional<ISO3166>;

		private:
			value_type value_;

			constexpr ISO3166() noexcept
				: value_{0} {}

			constexpr explicit ISO3166(const value_type value) noexcept
				: value_{value} {}

		public:
			[[nodiscard]] constexpr auto value() noexcept -> value_type& { return value_; }

			[[nodiscard]] constexpr auto value() const noexcept -> value_type { return value_; }

			[[nodiscard]] constexpr auto empty() const noexcept -> bool { return value() == 0; }

			[[nodiscard]] constexpr auto matches(const ISO3166& other) const noexcept -> bool { return empty() or *this == other; }

			[[nodiscard]] constexpr auto code2() const noexcept -> std::basic_string<element_type>;

			[[nodiscard]] constexpr auto code3() const noexcept -> std::basic_string<element_type>;

			[[nodiscard]] constexpr auto number() const noexcept -> value_type { return value(); }

			[[nodiscard]] friend constexpr auto operator==(const ISO3166&, const ISO3166&) noexcept -> bool = default;
			[[nodiscard]] friend constexpr auto operator<=>(const ISO3166&, const ISO3166&) noexcept        = default;
		};
	}

	[[nodiscard]] consteval auto make_iso_3166_code_info_database() noexcept -> auto
	{
		// https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes#Current_ISO_3166_country_codes
		constexpr ISO3166::code_info data[]{
				{"AF", "AFG", 4},
				{"AL", "ALB", 8},
				{"AQ", "ATA", 10},
				{"DZ", "DZA", 12},
				{"AS", "ASM", 16},
				{"AD", "AND", 20},
				{"AO", "AGO", 24},
				{"AG", "ATG", 28},
				{"AZ", "AZE", 31},
				{"AR", "ARG", 32},
				{"AU", "AUS", 36},
				{"AT", "AUT", 40},
				{"BS", "BHS", 44},
				{"BH", "BHR", 48},
				{"BD", "BGD", 50},
				{"AM", "ARM", 51},
				{"BB", "BRB", 52},
				{"BE", "BEL", 56},
				{"BM", "BMU", 60},
				{"BT", "BTN", 64},
				{"BO", "BOL", 68},
				{"BA", "BIH", 70},
				{"BW", "BWA", 72},
				{"BV", "BVT", 74},
				{"BR", "BRA", 76},
				{"BZ", "BLZ", 84},
				{"IO", "IOT", 86},
				{"SB", "SLB", 90},
				{"VG", "VGB", 92},
				{"BN", "BRN", 96},
				{"BG", "BGR", 100},
				{"MM", "MMR", 104},
				{"BI", "BDI", 108},
				{"BY", "BLR", 112},
				{"KH", "KHM", 116},
				{"CM", "CMR", 120},
				{"CA", "CAN", 124},
				{"CV", "CPV", 132},
				{"KY", "CYM", 136},
				{"CF", "CAF", 140},
				{"LK", "LKA", 144},
				{"TD", "TCD", 148},
				{"CL", "CHL", 152},
				{"CN", "CHN", 156},
				{"TW", "TWN", 158},
				{"CX", "CXR", 162},
				{"CC", "CCK", 166},
				{"CO", "COL", 170},
				{"KM", "COM", 174},
				{"YT", "MYT", 175},
				{"CG", "COG", 178},
				{"CD", "COD", 180},
				{"CK", "COK", 184},
				{"CR", "CRI", 188},
				{"HR", "HRV", 191},
				{"CU", "CUB", 192},
				{"CY", "CYP", 196},
				{"CZ", "CZE", 203},
				{"BJ", "BEN", 204},
				{"DK", "DNK", 208},
				{"DM", "DMA", 212},
				{"DO", "DOM", 214},
				{"EC", "ECU", 218},
				{"SV", "SLV", 222},
				{"GQ", "GNQ", 226},
				{"ET", "ETH", 231},
				{"ER", "ERI", 232},
				{"EE", "EST", 233},
				{"FO", "FRO", 234},
				{"FK", "FLK", 238},
				{"GS", "SGS", 239},
				{"FJ", "FJI", 242},
				{"FI", "FIN", 246},
				{"AX", "ALA", 248},
				{"FR", "FRA", 250},
				{"GF", "GUF", 254},
				{"PF", "PYF", 258},
				{"TF", "ATF", 260},
				{"DJ", "DJI", 262},
				{"GA", "GAB", 266},
				{"GE", "GEO", 268},
				{"GM", "GMB", 270},
				{"PS", "PSE", 275},
				{"DE", "DEU", 276},
				{"GH", "GHA", 288},
				{"GI", "GIB", 292},
				{"KI", "KIR", 296},
				{"GR", "GRC", 300},
				{"GL", "GRL", 304},
				{"GD", "GRD", 308},
				{"GP", "GLP", 312},
				{"GU", "GUM", 316},
				{"GT", "GTM", 320},
				{"GN", "GIN", 324},
				{"GY", "GUY", 328},
				{"HT", "HTI", 332},
				{"HM", "HMD", 334},
				{"VA", "VAT", 336},
				{"HN", "HND", 340},
				{"HK", "HKG", 344},
				{"HU", "HUN", 348},
				{"IS", "ISL", 352},
				{"IN", "IND", 356},
				{"ID", "IDN", 360},
				{"IR", "IRN", 364},
				{"IQ", "IRQ", 368},
				{"IE", "IRL", 372},
				{"IL", "ISR", 376},
				{"IT", "ITA", 380},
				{"CI", "CIV", 384},
				{"JM", "JAM", 388},
				{"JP", "JPN", 392},
				{"KZ", "KAZ", 398},
				{"JO", "JOR", 400},
				{"KE", "KEN", 404},
				{"KP", "PRK", 408},
				{"KR", "KOR", 410},
				{"KW", "KWT", 414},
				{"KG", "KGZ", 417},
				{"LA", "LAO", 418},
				{"LB", "LBN", 422},
				{"LS", "LSO", 426},
				{"LV", "LVA", 428},
				{"LR", "LBR", 430},
				{"LY", "LBY", 434},
				{"LI", "LIE", 438},
				{"LT", "LTU", 440},
				{"LU", "LUX", 442},
				{"MO", "MAC", 446},
				{"MG", "MDG", 450},
				{"MW", "MWI", 454},
				{"MY", "MYS", 458},
				{"MV", "MDV", 462},
				{"ML", "MLI", 466},
				{"MT", "MLT", 470},
				{"MQ", "MTQ", 474},
				{"MR", "MRT", 478},
				{"MU", "MUS", 480},
				{"MX", "MEX", 484},
				{"MC", "MCO", 492},
				{"MN", "MNG", 496},
				{"MD", "MDA", 498},
				{"ME", "MNE", 499},
				{"MS", "MSR", 500},
				{"MA", "MAR", 504},
				{"MZ", "MOZ", 508},
				{"OM", "OMN", 512},
				{"NA", "NAM", 516},
				{"NR", "NRU", 520},
				{"NP", "NPL", 524},
				{"NL", "NLD", 528},
				{"CW", "CUW", 531},
				{"AW", "ABW", 533},
				{"SX", "SXM", 534},
				{"BQ", "BES", 535},
				{"NC", "NCL", 540},
				{"VU", "VUT", 548},
				{"NZ", "NZL", 554},
				{"NI", "NIC", 558},
				{"NE", "NER", 562},
				{"NG", "NGA", 566},
				{"NU", "NIU", 570},
				{"NF", "NFK", 574},
				{"NO", "NOR", 578},
				{"MP", "MNP", 580},
				{"UM", "UMI", 581},
				{"FM", "FSM", 583},
				{"MH", "MHL", 584},
				{"PW", "PLW", 585},
				{"PK", "PAK", 586},
				{"PA", "PAN", 591},
				{"PG", "PNG", 598},
				{"PY", "PRY", 600},
				{"PE", "PER", 604},
				{"PH", "PHL", 608},
				{"PN", "PCN", 612},
				{"PL", "POL", 616},
				{"PT", "PRT", 620},
				{"GW", "GNB", 624},
				{"TL", "TLS", 626},
				{"PR", "PRI", 630},
				{"QA", "QAT", 634},
				{"RE", "REU", 638},
				{"RO", "ROU", 642},
				{"RU", "RUS", 643},
				{"RW", "RWA", 646},
				{"BL", "BLM", 652},
				{"SH", "SHN", 654},
				{"KN", "KNA", 659},
				{"AI", "AIA", 660},
				{"LC", "LCA", 662},
				{"MF", "MAF", 663},
				{"PM", "SPM", 666},
				{"VC", "VCT", 670},
				{"SM", "SMR", 674},
				{"ST", "STP", 678},
				{"SA", "SAU", 682},
				{"SN", "SEN", 686},
				{"RS", "SRB", 688},
				{"SC", "SYC", 690},
				{"SL", "SLE", 694},
				{"SG", "SGP", 702},
				{"SK", "SVK", 703},
				{"VN", "VNM", 704},
				{"SI", "SVN", 705},
				{"SO", "SOM", 706},
				{"ZA", "ZAF", 710},
				{"ZW", "ZWE", 716},
				{"ES", "ESP", 724},
				{"SS", "SSD", 728},
				{"SD", "SDN", 729},
				{"EH", "ESH", 732},
				{"SR", "SUR", 740},
				{"SJ", "SJM", 744},
				{"SZ", "SWZ", 748},
				{"SE", "SWE", 752},
				{"CH", "CHE", 756},
				{"SY", "SYR", 760},
				{"TJ", "TJK", 762},
				{"TH", "THA", 764},
				{"TG", "TGO", 768},
				{"TK", "TKL", 772},
				{"TO", "TON", 776},
				{"TT", "TTO", 780},
				{"AE", "ARE", 784},
				{"TN", "TUN", 788},
				{"TR", "TUR", 792},
				{"TM", "TKM", 795},
				{"TC", "TCA", 796},
				{"TV", "TUV", 798},
				{"UG", "UGA", 800},
				{"UA", "UKR", 804},
				{"MK", "MKD", 807},
				{"EG", "EGY", 818},
				{"GB", "GBR", 826},
				{"GG", "GGY", 831},
				{"JE", "JEY", 832},
				{"IM", "IMN", 833},
				{"TZ", "TZA", 834},
				{"US", "USA", 840},
				{"VI", "VIR", 850},
				{"BF", "BFA", 854},
				{"UY", "URY", 858},
				{"UZ", "UZB", 860},
				{"VE", "VEN", 862},
				{"WF", "WLF", 876},
				{"WS", "WSM", 882},
				{"YE", "YEM", 887},
				{"ZM", "ZMB", 894}};

		std::array<ISO3166::code_info, std::ranges::size(data)> result{};
		std::ranges::copy(data, result.data());

		return result;
	}

	constexpr auto iso_3166_code_info_database      = make_iso_3166_code_info_database();
	constexpr auto iso_3166_code_info_database_size = std::ranges::size(iso_3166_code_info_database);

	// fixme:
	// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
	//	          with
	//	          [
	//	              _Ty=std::pair<gal::prometheus::infrastructure::basic_fixed_string<char,2>,unsigned short>
	//	          ]
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(662): note: maybe "std::tuple_size<std::pair<_Ty1,_Ty2>>"
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(662): note: or        "std::tuple_size<std::pair<_Ty1,_Ty2>>"
	// using iso_3166_code2_to_number_mapping = std::pair<ISO3166::code_info_code2_type, ISO3166::value_type>;
	// using iso_3166_code3_to_number_mapping = std::pair<ISO3166::code_info_code3_type, ISO3166::value_type>;
	struct iso_3166_code2_to_number_mapping
	{
		ISO3166::code_info_code2_type first;
		ISO3166::value_type           second;
	};

	struct iso_3166_code3_to_number_mapping
	{
		ISO3166::code_info_code3_type first;
		ISO3166::value_type           second;
	};

	[[nodiscard]] consteval auto make_iso_3166_code_info_mapping_number_to_code2() noexcept -> std::array<ISO3166::code_info_code2_type, ISO3166::code_info_max_size>
	{
		std::array<ISO3166::code_info_code2_type, ISO3166::code_info_max_size> data{};
		std::ranges::fill(data, "??");

		std::ranges::for_each(
				iso_3166_code_info_database,
				[&data](const ISO3166::code_info& info) noexcept -> void { data[info.number] = info.code2; });

		return data;
	}

	[[nodiscard]] consteval auto make_iso_3166_code_info_mapping_number_to_code3() noexcept -> std::array<ISO3166::code_info_code3_type, ISO3166::code_info_max_size>
	{
		std::array<ISO3166::code_info_code3_type, ISO3166::code_info_max_size> data{};
		std::ranges::fill(data, "???");

		std::ranges::for_each(
				iso_3166_code_info_database,
				[&data](const ISO3166::code_info& info) noexcept -> void { data[info.number] = info.code3; });

		return data;
	}

	[[nodiscard]] consteval auto make_iso_3166_code_info_mapping_code2_to_number() noexcept -> std::array<iso_3166_code2_to_number_mapping, iso_3166_code_info_database_size>
	{
		std::array<iso_3166_code2_to_number_mapping, iso_3166_code_info_database_size> data{};

		std::ranges::for_each(
				std::views::zip(data, iso_3166_code_info_database),
				[](std::tuple<iso_3166_code2_to_number_mapping&, const ISO3166::code_info&> tuple) noexcept -> void
				{
					auto& [d, info] = tuple;

					d = iso_3166_code2_to_number_mapping{info.code2, info.number};
				});

		std::ranges::sort(
				data,
				std::ranges::less{},
				&iso_3166_code2_to_number_mapping::first);

		return data;
	}

	[[nodiscard]] consteval auto make_iso_3166_code_info_mapping_code3_to_number() noexcept -> std::array<iso_3166_code3_to_number_mapping, iso_3166_code_info_database_size>
	{
		std::array<iso_3166_code3_to_number_mapping, iso_3166_code_info_database_size> data{};

		std::ranges::for_each(
				std::views::zip(data, iso_3166_code_info_database),
				[](std::tuple<iso_3166_code3_to_number_mapping&, const ISO3166::code_info&> tuple) noexcept -> void
				{
					auto& [d, info] = tuple;

					d = iso_3166_code3_to_number_mapping{info.code3, info.number};
				});

		std::ranges::sort(
				data,
				std::ranges::less{},
				&iso_3166_code3_to_number_mapping::first);

		return data;
	}

	constexpr auto iso_3166_code_info_mapping_number_to_code2 = make_iso_3166_code_info_mapping_number_to_code2();
	constexpr auto iso_3166_code_info_mapping_number_to_code3 = make_iso_3166_code_info_mapping_number_to_code3();
	constexpr auto iso_3166_code_info_mapping_code2_to_number = make_iso_3166_code_info_mapping_code2_to_number();
	constexpr auto iso_3166_code_info_mapping_code3_to_number = make_iso_3166_code_info_mapping_code3_to_number();

	export
	{
		constexpr auto ISO3166::parse(const std::basic_string_view<element_type> string) noexcept -> std::optional<ISO3166>
		{
			// number
			if (infrastructure::is_digit(string))
			{
				if (const auto result = infrastructure::from_string<value_type, false>(string);
					result.has_value())
				{
					// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
					// 		*result < code_info_max_size,
					// 		"ISO-3166 number must be between 001 and 999, got '{}'",
					// 		string);

					if (*result < code_info_max_size) { return ISO3166{*result}; }
				}

				return std::nullopt;
			}

			// code
			if (infrastructure::is_alpha(string))
			{
				// code2
				if (string.size() == 2)
				{
					const auto upper_string = infrastructure::to_upper(string);

					// ReSharper disable once CppTooWideScopeInitStatement
					const auto it = std::ranges::lower_bound(
							iso_3166_code_info_mapping_code2_to_number,
							upper_string,
							std::ranges::less{},
							&iso_3166_code2_to_number_mapping::first
							);

					// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
					// 		it != iso_3166_code_info_mapping_code2_to_number.end() and it->first == upper_string,
					// 		"Invalid ISO-3166 2 letter language code '{}'",
					// 		string);

					if (it != iso_3166_code_info_mapping_code2_to_number.end() and it->first == upper_string) { return ISO3166{it->second}; }

					return std::nullopt;
				}

				// code3
				if (string.size() == 3)
				{
					const auto upper_string = infrastructure::to_upper(string);

					// ReSharper disable once CppTooWideScopeInitStatement
					const auto it = std::ranges::lower_bound(
							iso_3166_code_info_mapping_code3_to_number,
							upper_string,
							std::ranges::less{},
							&iso_3166_code3_to_number_mapping::first);

					// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
					// 		it != iso_3166_code_info_mapping_code3_to_number.end() and it->first == upper_string,
					// 		"Invalid ISO-3166 3 letter language code '{}'",
					// 		string);

					if (it != iso_3166_code_info_mapping_code3_to_number.end() and it->first == upper_string) { return ISO3166{it->second}; }

					return std::nullopt;
				}
			}

			return std::nullopt;
		}

		constexpr auto ISO3166::code2() const noexcept -> std::basic_string<element_type>
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(value() < code_info_max_size);

			const auto result = iso_3166_code_info_mapping_number_to_code2[static_cast<decltype(iso_3166_code_info_mapping_number_to_code2)::size_type>(value())];
			return result.operator std::basic_string<element_type>();
		}

		constexpr auto ISO3166::code3() const noexcept -> std::basic_string<element_type>
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(value() < code_info_max_size);

			const auto result = iso_3166_code_info_mapping_number_to_code3[static_cast<decltype(iso_3166_code_info_mapping_number_to_code3)::size_type>(value())];
			return result.operator std::basic_string<element_type>();
		}
	}
}

export namespace std
{
	template<>
	struct hash<gal::prometheus::i18n::ISO3166>
	{
		[[nodiscard]] auto operator()(const gal::prometheus::i18n::ISO3166& i) const noexcept -> std::size_t { return hash<gal::prometheus::i18n::ISO3166::value_type>{}(i.value()); }
	};
}
