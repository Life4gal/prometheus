// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.i18n:iso_15924;

import std;
import gal.prometheus.infrastructure;

namespace gal::prometheus::i18n
{
	export
	{
		class IETFLanguageTag;

		/**
		 * @brief ISO-15924 script code.
		 */
		// ReSharper disable once CppInconsistentNaming
		class ISO15924
		{
			friend IETFLanguageTag;

		public:
			using value_type = std::uint16_t;

			using element_type = char;

			using code_info_code4_type = infrastructure::fixed_string<4>;

			struct code_info
			{
				code_info_code4_type code4;
				value_type           number;

				constexpr code_info() noexcept
					: code4{"????"},
					number{9999} {}

				// https://en.wikipedia.org/wiki/IETF_language_tag#Syntax_of_language_tags
				// An optional script subtag, based on a four-letter script code from ISO 15924 (usually written in Title Case);
				constexpr code_info(const element_type (&c)[5], const value_type n) noexcept
					: code4{infrastructure::to_tittle(code_info_code4_type{c})},
					number{n} { }
			};

			// 001 ~ 999
			constexpr static auto code_info_max_size = 1000;

			constexpr static auto parse(std::basic_string_view<element_type> string) noexcept -> std::optional<ISO15924>;

		private:
			value_type value_;

			constexpr ISO15924() noexcept
				: value_{0} {}

			constexpr explicit ISO15924(const value_type value) noexcept
				: value_{value} {}

		public:
			[[nodiscard]] constexpr auto value() noexcept -> value_type& { return value_; }

			[[nodiscard]] constexpr auto value() const noexcept -> value_type { return value_; }

			[[nodiscard]] constexpr auto empty() const noexcept -> bool { return value() == 0; }

			[[nodiscard]] constexpr auto matches(const ISO15924& other) const noexcept -> bool { return empty() or *this == other; }

			[[nodiscard]] constexpr auto code4() const noexcept -> std::basic_string<element_type>;

			[[nodiscard]] constexpr auto number() const noexcept -> value_type { return value(); }

			[[nodiscard]] friend constexpr auto operator==(const ISO15924&, const ISO15924&) noexcept -> bool = default;
			[[nodiscard]] friend constexpr auto operator<=>(const ISO15924&, const ISO15924&) noexcept        = default;
		};
	}

	[[nodiscard]] consteval auto make_iso_15924_code_info_database() noexcept -> auto
	{
		// https://en.wikipedia.org/wiki/ISO_15924#List_of_codes
		constexpr ISO15924::code_info data[]
		{
				{"adlm", 166},
				{"afak", 439},
				{"aghb", 239},
				{"ahom", 338},
				{"arab", 160},
				{"aran", 161},
				{"armi", 124},
				{"armn", 230},
				{"avst", 134},
				{"bali", 360},
				{"bamu", 435},
				{"bass", 259},
				{"batk", 365},
				{"beng", 325},
				{"bhks", 334},
				{"blis", 550},
				{"bopo", 285},
				{"brah", 300},
				{"brai", 570},
				{"bugi", 367},
				{"buhd", 372},
				{"cakm", 349},
				{"cans", 440},
				{"cari", 201},
				{"cham", 358},
				{"cher", 445},
				{"chrs", 109},
				{"cirt", 291},
				{"copt", 204},
				{"cpmn", 402},
				{"cprt", 403},
				{"cyrl", 220},
				{"cyrs", 221},
				{"deva", 315},
				{"diak", 342},
				{"dogr", 328},
				{"dsrt", 250},
				{"dupl", 755},
				{"egyd", 70},
				{"egyh", 60},
				{"egyp", 50},
				{"elba", 226},
				{"elym", 128},
				{"ethi", 430},
				{"geok", 241},
				{"geor", 240},
				{"glag", 225},
				{"gong", 312},
				{"gonm", 313},
				{"goth", 206},
				{"gran", 343},
				{"grek", 200},
				{"gujr", 320},
				{"guru", 310},
				{"hanb", 503},
				{"hang", 286},
				{"hani", 500},
				{"hano", 371},
				{"hans", 501},
				{"hant", 502},
				{"hatr", 127},
				{"hebr", 125},
				{"hira", 410},
				{"hluw", 80},
				{"hmng", 450},
				{"hmnp", 451},
				{"hrkt", 412},
				{"hung", 176},
				{"inds", 610},
				{"ital", 210},
				{"jamo", 284},
				{"java", 361},
				{"jpan", 413},
				{"jurc", 510},
				{"kali", 357},
				{"kana", 411},
				{"khar", 305},
				{"khmr", 355},
				{"khoj", 322},
				{"kitl", 505},
				{"kits", 288},
				{"knda", 345},
				{"kore", 287},
				{"kpel", 436},
				{"kthi", 317},
				{"lana", 351},
				{"laoo", 356},
				{"latf", 217},
				{"latg", 216},
				{"latn", 215},
				{"leke", 364},
				{"lepc", 335},
				{"limb", 336},
				{"lina", 400},
				{"linb", 401},
				{"lisu", 399},
				{"loma", 437},
				{"lyci", 202},
				{"lydi", 116},
				{"mahj", 314},
				{"maka", 366},
				{"mand", 140},
				{"mani", 139},
				{"marc", 332},
				{"maya", 90},
				{"medf", 265},
				{"mend", 438},
				{"merc", 101},
				{"mero", 100},
				{"mlym", 347},
				{"modi", 324},
				{"mong", 145},
				{"moon", 218},
				{"mroo", 264},
				{"mtei", 337},
				{"mult", 323},
				{"mymr", 350},
				{"nand", 311},
				{"narb", 106},
				{"nbat", 159},
				{"newa", 333},
				{"nkdb", 85},
				{"nkgb", 420},
				{"nkoo", 165},
				{"nshu", 499},
				{"ogam", 212},
				{"olck", 261},
				{"orkh", 175},
				{"orya", 327},
				{"osge", 219},
				{"osma", 260},
				{"ougr", 143},
				{"palm", 126},
				{"pauc", 263},
				{"pcun", 15},
				{"pelm", 16},
				{"perm", 227},
				{"phag", 331},
				{"phli", 131},
				{"phlp", 132},
				{"phlv", 133},
				{"phnx", 115},
				{"plrd", 282},
				{"piqd", 293},
				{"prti", 130},
				{"psin", 103},
				{"qaaa", 900},
				{"qabv", 947},
				{"qabw", 948},
				{"qabx", 949},
				{"ranj", 303},
				{"rjng", 363},
				{"rohg", 167},
				{"roro", 620},
				{"runr", 211},
				{"samr", 123},
				{"sara", 292},
				{"sarb", 105},
				{"saur", 344},
				{"sgnw", 95},
				{"shaw", 281},
				{"shrd", 319},
				{"shui", 530},
				{"sidd", 302},
				{"sind", 318},
				{"sinh", 348},
				{"sogd", 141},
				{"sogo", 142},
				{"sora", 398},
				{"soyo", 329},
				{"sund", 362},
				{"sylo", 316},
				{"syrc", 135},
				{"syre", 138},
				{"syrj", 137},
				{"syrn", 136},
				{"tagb", 373},
				{"takr", 321},
				{"tale", 353},
				{"talu", 354},
				{"taml", 346},
				{"tang", 520},
				{"tavt", 359},
				{"telu", 340},
				{"teng", 290},
				{"tfng", 120},
				{"tglg", 370},
				{"thaa", 170},
				{"thai", 352},
				{"tibt", 330},
				{"tirh", 326},
				{"tnsa", 275},
				{"toto", 294},
				{"ugar", 40},
				{"vaii", 470},
				{"visp", 280},
				{"vith", 228},
				{"wara", 262},
				{"wcho", 283},
				{"wole", 480},
				{"xpeo", 30},
				{"xsux", 20},
				{"yezi", 192},
				{"yiii", 460},
				{"zanb", 339},
				{"zinh", 994},
				{"zmth", 995},
				{"zsye", 993},
				{"zsym", 996},
				{"zxxx", 997},
				{"zyyy", 998},
				{"zzzz", 999}};

		std::array<ISO15924::code_info, std::ranges::size(data)> result{};
		std::ranges::copy(data, result.data());

		return result;
	}

	constexpr auto iso_15924_code_info_database      = make_iso_15924_code_info_database();
	constexpr auto iso_15924_code_info_database_size = std::ranges::size(iso_15924_code_info_database);

	// fixme:
	// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
	//	          with
	//	          [
	//	              _Ty=std::pair<gal::prometheus::infrastructure::basic_fixed_string<char,4>,unsigned short>
	//	          ]
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(662): note: maybe "std::tuple_size<std::pair<_Ty1,_Ty2>>"
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(662): note: or        "std::tuple_size<std::pair<_Ty1,_Ty2>>"
	// using iso_15924_code4_to_number_mapping = std::pair<ISO15924::code_info_code4_type, ISO15924::value_type>;
	struct iso_15924_code4_to_number_mapping
	{
		ISO15924::code_info_code4_type first;
		ISO15924::value_type           second;
	};

	[[nodiscard]] consteval auto make_iso_15924_code_info_mapping_number_to_code4() noexcept -> std::array<ISO15924::code_info_code4_type, ISO15924::code_info_max_size>
	{
		std::array<ISO15924::code_info_code4_type, ISO15924::code_info_max_size> data{};
		std::ranges::fill(data, "????");

		std::ranges::for_each(
				iso_15924_code_info_database,
				[&data](const ISO15924::code_info& info) noexcept -> void { data[info.number] = info.code4; });

		return data;
	}

	[[nodiscard]] consteval auto make_iso_15924_code_info_mapping_code4_to_number() noexcept -> std::array<iso_15924_code4_to_number_mapping, iso_15924_code_info_database_size>
	{
		std::array<iso_15924_code4_to_number_mapping, iso_15924_code_info_database_size> data{};

		std::ranges::for_each(
				std::views::zip(data, iso_15924_code_info_database),
				[](std::tuple<iso_15924_code4_to_number_mapping&, const ISO15924::code_info&> tuple) noexcept -> void
				{
					auto& [d, info] = tuple;

					d = iso_15924_code4_to_number_mapping{info.code4, info.number};
				});

		std::ranges::sort(
				data,
				std::ranges::less{},
				&iso_15924_code4_to_number_mapping::first);

		return data;
	}

	constexpr auto iso_15924_code_info_mapping_number_to_code4 = make_iso_15924_code_info_mapping_number_to_code4();
	constexpr auto iso_15924_code_info_mapping_code4_to_number = make_iso_15924_code_info_mapping_code4_to_number();

	export
	{
		constexpr auto ISO15924::parse(const std::basic_string_view<element_type> string) noexcept -> std::optional<ISO15924>
		{
			// number
			if (infrastructure::is_digit(string))
			{
				if (const auto result = infrastructure::from_string<value_type, false>(string);
					result.has_value())
				{
					// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
					// 		*result < code_info_max_size,
					// 		"ISO-15924 number must be between 001 and 999, got '{}'",
					// 		string);

					if (*result < code_info_max_size) { return ISO15924{*result}; }
				}

				return std::nullopt;
			}

			// code
			if (infrastructure::is_alpha(string) and string.size() == 4)
			{
				const auto target = infrastructure::to_tittle(string);

				// ReSharper disable once CppTooWideScopeInitStatement
				const auto it = std::ranges::lower_bound(
						iso_15924_code_info_mapping_code4_to_number,
						target,
						std::ranges::less{},
						&iso_15924_code4_to_number_mapping::first);

				// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
				// 		it != iso_15924_code_info_mapping_code4_to_number.end() and it->first == target,
				// 		"Invalid ISO-15924 script code '{}'",
				// 		target);

				if (it != iso_15924_code_info_mapping_code4_to_number.end() and it->first == target) { return ISO15924{it->second}; }

				return std::nullopt;
			}

			return std::nullopt;
		}

		constexpr auto ISO15924::code4() const noexcept -> std::basic_string<element_type>
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(value() < code_info_max_size);

			const auto result = iso_15924_code_info_mapping_number_to_code4[static_cast<decltype(iso_15924_code_info_mapping_number_to_code4)::size_type>(value())];
			return result.operator std::basic_string<element_type>();
		}
	}
}

export namespace std
{
	template<>
	struct hash<gal::prometheus::i18n::ISO15924>
	{
		[[nodiscard]] auto operator()(const gal::prometheus::i18n::ISO15924& i) const noexcept -> std::size_t { return hash<gal::prometheus::i18n::ISO15924::value_type>{}(i.value()); }
	};
}// namespace std
