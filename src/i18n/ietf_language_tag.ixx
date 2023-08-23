// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.i18n:ietf_language_tag;

import std;
import gal.prometheus.infrastructure;

import :iso_639;
import :iso_15924;
import :iso_3166;

namespace gal::prometheus::i18n
{
	export
	{
		/**
		 * @brief The IETF BCP 47 language tag.
		 *
		 * @note This class stores the language tag in 64 bits; in its individual components of the:
		 *	1.ISO-639 language (16 bit)
		 *	2.ISO-3166 region (16 bit)
		 *	3.ISO-15924 script (16 bit)
		 *	4.Reserved for future variants or extensions
		 */
		// ReSharper disable once CppInconsistentNaming
		class IETFLanguageTag
		{
		public:
			using value_type = std::uint16_t;

			using element_type = char;

			using language_type = ISO639;
			using region_type = ISO3166;
			using script_type = ISO15924;

			struct tag_info
			{
				std::basic_string_view<element_type> from;
				std::basic_string_view<element_type> to;
			};

			constexpr static auto parse(std::basic_string_view<element_type> string) noexcept -> std::optional<IETFLanguageTag>;

		private:
			language_type language_;
			region_type   region_;
			script_type   script_;
			value_type    reserved_{0};

			constexpr explicit IETFLanguageTag(const language_type language, const region_type region = {}, const script_type script = {}) noexcept
				: language_{language},
				region_{region},
				script_{script} {}

		public:
			[[nodiscard]] constexpr explicit operator std::basic_string<element_type>() const noexcept
			{
				std::basic_string<element_type> result{};
				result.reserve(
						3// language
						+
						4// script
						+
						2// region
						+
						2// '-'
						);

				result += language_.code();
				if (not script_.empty())
				{
					result.push_back('-');
					result += script_.code4();
				}
				if (not region_.empty())
				{
					result.push_back('-');
					result += region_.code2();
				}

				return result;
			}

			[[nodiscard]] constexpr auto operator==(const IETFLanguageTag& other) const noexcept -> bool = default;

			/**
			 * @brief Check if two language_tags match for their non-empty fields.
			 */
			[[nodiscard]] constexpr auto matches(const IETFLanguageTag& other) const noexcept -> bool
			{
				if (not language_.empty() and not other.language_.empty() and language_ != other.language_) { return false; }

				if (not script_.empty() and not other.script_.empty() and script_ != other.script_) { return false; }

				if (not region_.empty() and not other.region_.empty() and region_ != other.region_) { return false; }

				return true;
			}

			/**
			 * @brief Expand the language tag to include script and language.
			 * 
			 * @note Expansion is done by querying default script, default language and grandfathering tables.
			 */
			[[nodiscard]] constexpr auto expand() const noexcept -> IETFLanguageTag;

			/**
			 * @brief Get a tag with only the language.
			 */
			[[nodiscard]] constexpr auto shrink() const noexcept -> IETFLanguageTag;

			/**
			 * @brief Get variants of the language tag.
			 * @return A list of language tags sorted: lang-script-region, lang-region, lang-script, lang.
			 *
			 * @note This function will create language_tags that includes this tag and tags with strictly less information (no script, no region).
			 */
			template<std::ranges::range Container = std::vector<IETFLanguageTag>>
				requires std::is_same_v<std::ranges::range_value_t<Container>, IETFLanguageTag> and
						std::is_default_constructible_v<Container> and
						requires
						{
							std::declval<Container&>().emplace_back(std::declval<IETFLanguageTag&&>());
						}
			[[nodiscard]] constexpr auto variants() const -> Container
			{
				Container result{};

				result.emplace_back(*this);

				if (not script_.empty() and not region_.empty())
				{
					result.emplace_back(IETFLanguageTag{language_, region_, script_type{}});
					result.emplace_back(IETFLanguageTag{language_, region_type{}, script_});
				}
				if (not script_.empty() or not region_.empty()) { result.emplace_back(IETFLanguageTag{language_, region_type{}, script_type{}}); }

				return result;
			}

			/**
			 * @brief Get variants of the language tag.
			 * @return A list of language tags sorted: lang-script-region, lang-region, lang-script, lang.
			 *
			 * @note This function will create language_tags that includes this tag and tags with strictly less information (no script, no region), which still canonically expands into this tag.
			 */
			template<std::ranges::range Container = std::vector<IETFLanguageTag>>
				requires
				std::is_same_v<std::ranges::range_value_t<Container>, IETFLanguageTag> and
				std::is_default_constructible_v<Container> and
				requires
				{
					std::declval<Container&>().emplace_back(std::declval<IETFLanguageTag&&>());
				}
			[[nodiscard]] constexpr auto canonical_variants() const -> Container
			{
				Container result{};

				// fixme:
				// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
				//	          with
				//	          [
				//	              _Ty=std::tuple<gal::prometheus::i18n::IETFLanguageTag::canonical_variants::<lambda_1>>
				//	          ]
				//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: maybe "std::tuple_size<std::tuple<_Types...>>"
				//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: or        "std::tuple_size<std::tuple<_Types...>>"
				// for (const auto vs = variants(); const auto& tag: vs | std::views::filter([target = expand()](const IETFLanguageTag t) -> bool { return t.expand() == target; })) { result.emplace_back(tag); }

				const auto target = expand();
				for (
					const auto  vs = variants<Container>();
					const auto& tag: vs) { if (tag.expand() == target) { result.emplace_back(tag); } }

				return result;
			}

			/**
			 * @brief Creates variants of a language tag, including those by expanding the normal variants.
			 */
			template<std::ranges::range Container = std::vector<IETFLanguageTag>>
				requires std::is_same_v<std::ranges::range_value_t<Container>, IETFLanguageTag> and
						std::is_default_constructible_v<Container> and
						requires
						{
							std::declval<Container&>().emplace_back(std::declval<IETFLanguageTag&&>());
						}
			[[nodiscard]] constexpr auto all_variants() const -> Container
			{
				auto result = variants<Container>();

				// And languages variants from expanded variants.
				std::ranges::for_each(
						variants<Container>(),
						[&result](const IETFLanguageTag v) -> void
						{
							std::ranges::for_each(
									v.expand().variants<Container>(),
									[&result](const IETFLanguageTag ev) -> void { if (std::ranges::find(result, ev) == result.end()) { result.emplace_back(ev); } });
						});

				return result;
			}

			/**
			 * @brief Add variants to the list of languages.
			 * @param languages A list of languages ordered by preference.
			 * @return A new list of languages which includes variants and ordered by the given list of languages.
			 *
			 * @note This function is mostly used to add languages to a list of preferred languages to search for translations in the translation catalog.
			 */
			template<std::ranges::range Container>
				requires std::is_same_v<std::ranges::range_value_t<Container>, IETFLanguageTag> and
						requires(Container& container)
						{
							{
								std::declval<const IETFLanguageTag&>().all_variants<Container>()
							} -> std::same_as<Container>;
						}
			[[nodiscard]] constexpr static auto variants(const Container& languages) -> Container
			{
				Container result{};
				std::ranges::for_each(
						//
						languages |
						//
						std::views::transform([](const IETFLanguageTag& tag) -> Container { return tag.all_variants<Container>(); }) |
						//
						std::views::join |
						//
						std::views::common,
						[&result](const IETFLanguageTag& each) -> void { if (std::ranges::find(result, each) == result.end()) { result.emplace_back(each); } }
						);

				return result;
			}
		};
	}

	static_assert(sizeof(IETFLanguageTag) == sizeof(std::uint64_t));

	[[nodiscard]] consteval auto make_ietf_language_tag_tag_info_database() noexcept -> auto
	{
		// Most data from this table comes from unicode locale database: https://github.com/unicode-org/cldr/blob/main/common/supplemental/likelySubtags.xml
		// Also included is a modified grandfathered table.
		constexpr IETFLanguageTag::tag_info data[]{
				{"aa", "aa-Latn-ET"},
				{"aai", "aai-Latn-ZZ"},
				{"aak", "aak-Latn-ZZ"},
				{"aau", "aau-Latn-ZZ"},
				{"ab", "ab-Cyrl-GE"},
				{"abi", "abi-Latn-ZZ"},
				{"abq", "abq-Cyrl-ZZ"},
				{"abr", "abr-Latn-GH"},
				{"abt", "abt-Latn-ZZ"},
				{"aby", "aby-Latn-ZZ"},
				{"acd", "acd-Latn-ZZ"},
				{"ace", "ace-Latn-ID"},
				{"ach", "ach-Latn-UG"},
				{"ada", "ada-Latn-GH"},
				{"ade", "ade-Latn-ZZ"},
				{"adj", "adj-Latn-ZZ"},
				{"adp", "adp-Tibt-BT"},
				{"ady", "ady-Cyrl-RU"},
				{"adz", "adz-Latn-ZZ"},
				{"ae", "ae-Avst-IR"},
				{"aeb", "aeb-Arab-TN"},
				{"aey", "aey-Latn-ZZ"},
				{"af", "af-Latn-ZA"},
				{"agc", "agc-Latn-ZZ"},
				{"agd", "agd-Latn-ZZ"},
				{"agg", "agg-Latn-ZZ"},
				{"agm", "agm-Latn-ZZ"},
				{"ago", "ago-Latn-ZZ"},
				{"agq", "agq-Latn-CM"},
				{"aha", "aha-Latn-ZZ"},
				{"ahl", "ahl-Latn-ZZ"},
				{"aho", "aho-Ahom-IN"},
				{"ajg", "ajg-Latn-ZZ"},
				{"ak", "ak-Latn-GH"},
				{"akk", "akk-Xsux-IQ"},
				{"ala", "ala-Latn-ZZ"},
				{"ali", "ali-Latn-ZZ"},
				{"aln", "aln-Latn-XK"},
				{"alt", "alt-Cyrl-RU"},
				{"am", "am-Ethi-ET"},
				{"amm", "amm-Latn-ZZ"},
				{"amn", "amn-Latn-ZZ"},
				{"amo", "amo-Latn-NG"},
				{"amp", "amp-Latn-ZZ"},
				{"an", "an-Latn-ES"},
				{"anc", "anc-Latn-ZZ"},
				{"ank", "ank-Latn-ZZ"},
				{"ann", "ann-Latn-ZZ"},
				{"any", "any-Latn-ZZ"},
				{"aoj", "aoj-Latn-ZZ"},
				{"aom", "aom-Latn-ZZ"},
				{"aoz", "aoz-Latn-ID"},
				{"apc", "apc-Arab-ZZ"},
				{"apd", "apd-Arab-TG"},
				{"ape", "ape-Latn-ZZ"},
				{"apr", "apr-Latn-ZZ"},
				{"aps", "aps-Latn-ZZ"},
				{"apz", "apz-Latn-ZZ"},
				{"ar", "ar-Arab-EG"},
				{"arc", "arc-Armi-IR"},
				{"arc-nbat", "arc-Nbat-JO"},
				{"arc-palm", "arc-Palm-SY"},
				{"arh", "arh-Latn-ZZ"},
				{"arn", "arn-Latn-CL"},
				{"aro", "aro-Latn-BO"},
				{"arq", "arq-Arab-DZ"},
				{"ars", "ars-Arab-SA"},
				{"ary", "ary-Arab-MA"},
				{"arz", "arz-Arab-EG"},
				{"as", "as-Beng-IN"},
				{"asa", "asa-Latn-TZ"},
				{"ase", "ase-Sgnw-US"},
				{"asg", "asg-Latn-ZZ"},
				{"aso", "aso-Latn-ZZ"},
				{"ast", "ast-Latn-ES"},
				{"ata", "ata-Latn-ZZ"},
				{"atg", "atg-Latn-ZZ"},
				{"atj", "atj-Latn-CA"},
				{"auy", "auy-Latn-ZZ"},
				{"av", "av-Cyrl-RU"},
				{"avl", "avl-Arab-ZZ"},
				{"avn", "avn-Latn-ZZ"},
				{"avt", "avt-Latn-ZZ"},
				{"avu", "avu-Latn-ZZ"},
				{"awa", "awa-Deva-IN"},
				{"awb", "awb-Latn-ZZ"},
				{"awo", "awo-Latn-ZZ"},
				{"awx", "awx-Latn-ZZ"},
				{"ay", "ay-Latn-BO"},
				{"ayb", "ayb-Latn-ZZ"},
				{"az", "az-Latn-AZ"},
				{"az-arab", "az-Arab-IR"},
				{"az-iq", "az-Arab-IQ"},
				{"az-ir", "az-Arab-IR"},
				{"az-ru", "az-Cyrl-RU"},
				{"ba", "ba-Cyrl-RU"},
				{"bal", "bal-Arab-PK"},
				{"ban", "ban-Latn-ID"},
				{"bap", "bap-Deva-NP"},
				{"bar", "bar-Latn-AT"},
				{"bas", "bas-Latn-CM"},
				{"bav", "bav-Latn-ZZ"},
				{"bax", "bax-Bamu-CM"},
				{"bba", "bba-Latn-ZZ"},
				{"bbb", "bbb-Latn-ZZ"},
				{"bbc", "bbc-Latn-ID"},
				{"bbd", "bbd-Latn-ZZ"},
				{"bbj", "bbj-Latn-CM"},
				{"bbp", "bbp-Latn-ZZ"},
				{"bbr", "bbr-Latn-ZZ"},
				{"bcf", "bcf-Latn-ZZ"},
				{"bch", "bch-Latn-ZZ"},
				{"bci", "bci-Latn-CI"},
				{"bcm", "bcm-Latn-ZZ"},
				{"bcn", "bcn-Latn-ZZ"},
				{"bco", "bco-Latn-ZZ"},
				{"bcq", "bcq-Ethi-ZZ"},
				{"bcu", "bcu-Latn-ZZ"},
				{"bdd", "bdd-Latn-ZZ"},
				{"be", "be-Cyrl-BY"},
				{"bef", "bef-Latn-ZZ"},
				{"beh", "beh-Latn-ZZ"},
				{"bej", "bej-Arab-SD"},
				{"bem", "bem-Latn-ZM"},
				{"bet", "bet-Latn-ZZ"},
				{"bew", "bew-Latn-ID"},
				{"bex", "bex-Latn-ZZ"},
				{"bez", "bez-Latn-TZ"},
				{"bfd", "bfd-Latn-CM"},
				{"bfq", "bfq-Taml-IN"},
				{"bft", "bft-Arab-PK"},
				{"bfy", "bfy-Deva-IN"},
				{"bg", "bg-Cyrl-BG"},
				{"bgc", "bgc-Deva-IN"},
				{"bgn", "bgn-Arab-PK"},
				{"bgx", "bgx-Grek-TR"},
				{"bhb", "bhb-Deva-IN"},
				{"bhg", "bhg-Latn-ZZ"},
				{"bhi", "bhi-Deva-IN"},
				{"bhl", "bhl-Latn-ZZ"},
				{"bho", "bho-Deva-IN"},
				{"bhy", "bhy-Latn-ZZ"},
				{"bi", "bi-Latn-VU"},
				{"bib", "bib-Latn-ZZ"},
				{"big", "big-Latn-ZZ"},
				{"bik", "bik-Latn-PH"},
				{"bim", "bim-Latn-ZZ"},
				{"bin", "bin-Latn-NG"},
				{"bio", "bio-Latn-ZZ"},
				{"biq", "biq-Latn-ZZ"},
				{"bjh", "bjh-Latn-ZZ"},
				{"bji", "bji-Ethi-ZZ"},
				{"bjj", "bjj-Deva-IN"},
				{"bjn", "bjn-Latn-ID"},
				{"bjo", "bjo-Latn-ZZ"},
				{"bjr", "bjr-Latn-ZZ"},
				{"bjt", "bjt-Latn-SN"},
				{"bjz", "bjz-Latn-ZZ"},
				{"bkc", "bkc-Latn-ZZ"},
				{"bkm", "bkm-Latn-CM"},
				{"bkq", "bkq-Latn-ZZ"},
				{"bku", "bku-Latn-PH"},
				{"bkv", "bkv-Latn-ZZ"},
				{"blg", "blg-Latn-MY"},
				{"blt", "blt-Tavt-VN"},
				{"bm", "bm-Latn-ML"},
				{"bmh", "bmh-Latn-ZZ"},
				{"bmk", "bmk-Latn-ZZ"},
				{"bmq", "bmq-Latn-ML"},
				{"bmu", "bmu-Latn-ZZ"},
				{"bn", "bn-Beng-BD"},
				{"bng", "bng-Latn-ZZ"},
				{"bnm", "bnm-Latn-ZZ"},
				{"bnp", "bnp-Latn-ZZ"},
				{"bo", "bo-Tibt-CN"},
				{"boj", "boj-Latn-ZZ"},
				{"bom", "bom-Latn-ZZ"},
				{"bon", "bon-Latn-ZZ"},
				{"bpy", "bpy-Beng-IN"},
				{"bqc", "bqc-Latn-ZZ"},
				{"bqi", "bqi-Arab-IR"},
				{"bqp", "bqp-Latn-ZZ"},
				{"bqv", "bqv-Latn-CI"},
				{"br", "br-Latn-FR"},
				{"bra", "bra-Deva-IN"},
				{"brh", "brh-Arab-PK"},
				{"brx", "brx-Deva-IN"},
				{"brz", "brz-Latn-ZZ"},
				{"bs", "bs-Latn-BA"},
				{"bsj", "bsj-Latn-ZZ"},
				{"bsq", "bsq-Bass-LR"},
				{"bss", "bss-Latn-CM"},
				{"bst", "bst-Ethi-ZZ"},
				{"bto", "bto-Latn-PH"},
				{"btt", "btt-Latn-ZZ"},
				{"btv", "btv-Deva-PK"},
				{"bua", "bua-Cyrl-RU"},
				{"buc", "buc-Latn-YT"},
				{"bud", "bud-Latn-ZZ"},
				{"bug", "bug-Latn-ID"},
				{"buk", "buk-Latn-ZZ"},
				{"bum", "bum-Latn-CM"},
				{"buo", "buo-Latn-ZZ"},
				{"bus", "bus-Latn-ZZ"},
				{"buu", "buu-Latn-ZZ"},
				{"bvb", "bvb-Latn-GQ"},
				{"bwd", "bwd-Latn-ZZ"},
				{"bwr", "bwr-Latn-ZZ"},
				{"bxh", "bxh-Latn-ZZ"},
				{"bye", "bye-Latn-ZZ"},
				{"byn", "byn-Ethi-ER"},
				{"byr", "byr-Latn-ZZ"},
				{"bys", "bys-Latn-ZZ"},
				{"byv", "byv-Latn-CM"},
				{"byx", "byx-Latn-ZZ"},
				{"bza", "bza-Latn-ZZ"},
				{"bze", "bze-Latn-ML"},
				{"bzf", "bzf-Latn-ZZ"},
				{"bzh", "bzh-Latn-ZZ"},
				{"bzw", "bzw-Latn-ZZ"},
				{"ca", "ca-Latn-ES"},
				{"cad", "cad-Latn-US"},
				{"can", "can-Latn-ZZ"},
				{"cbj", "cbj-Latn-ZZ"},
				{"cch", "cch-Latn-NG"},
				{"ccp", "ccp-Cakm-BD"},
				{"ce", "ce-Cyrl-RU"},
				{"ceb", "ceb-Latn-PH"},
				{"cfa", "cfa-Latn-ZZ"},
				{"cgg", "cgg-Latn-UG"},
				{"ch", "ch-Latn-GU"},
				{"chk", "chk-Latn-FM"},
				{"chm", "chm-Cyrl-RU"},
				{"cho", "cho-Latn-US"},
				{"chp", "chp-Latn-CA"},
				{"chr", "chr-Cher-US"},
				{"cic", "cic-Latn-US"},
				{"cja", "cja-Arab-KH"},
				{"cjm", "cjm-Cham-VN"},
				{"cjv", "cjv-Latn-ZZ"},
				{"ckb", "ckb-Arab-IQ"},
				{"ckl", "ckl-Latn-ZZ"},
				{"cko", "cko-Latn-ZZ"},
				{"cky", "cky-Latn-ZZ"},
				{"cla", "cla-Latn-ZZ"},
				{"cme", "cme-Latn-ZZ"},
				{"cmg", "cmg-Soyo-MN"},
				{"co", "co-Latn-FR"},
				{"cop", "cop-Copt-EG"},
				{"cps", "cps-Latn-PH"},
				{"cr", "cr-Cans-CA"},
				{"crh", "crh-Cyrl-UA"},
				{"crj", "crj-Cans-CA"},
				{"crk", "crk-Cans-CA"},
				{"crl", "crl-Cans-CA"},
				{"crm", "crm-Cans-CA"},
				{"crs", "crs-Latn-SC"},
				{"cs", "cs-Latn-CZ"},
				{"csb", "csb-Latn-PL"},
				{"csw", "csw-Cans-CA"},
				{"ctd", "ctd-Pauc-MM"},
				{"cu", "cu-Cyrl-RU"},
				{"cu-glag", "cu-Glag-BG"},
				{"cv", "cv-Cyrl-RU"},
				{"cy", "cy-Latn-GB"},
				{"da", "da-Latn-DK"},
				{"dad", "dad-Latn-ZZ"},
				{"daf", "daf-Latn-CI"},
				{"dag", "dag-Latn-ZZ"},
				{"dah", "dah-Latn-ZZ"},
				{"dak", "dak-Latn-US"},
				{"dar", "dar-Cyrl-RU"},
				{"dav", "dav-Latn-KE"},
				{"dbd", "dbd-Latn-ZZ"},
				{"dbq", "dbq-Latn-ZZ"},
				{"dcc", "dcc-Arab-IN"},
				{"ddn", "ddn-Latn-ZZ"},
				{"de", "de-Latn-DE"},
				{"ded", "ded-Latn-ZZ"},
				{"den", "den-Latn-CA"},
				{"dga", "dga-Latn-ZZ"},
				{"dgh", "dgh-Latn-ZZ"},
				{"dgi", "dgi-Latn-ZZ"},
				{"dgl", "dgl-Arab-ZZ"},
				{"dgr", "dgr-Latn-CA"},
				{"dgz", "dgz-Latn-ZZ"},
				{"dia", "dia-Latn-ZZ"},
				{"dje", "dje-Latn-NE"},
				{"dmf", "dmf-Medf-NG"},
				{"dnj", "dnj-Latn-CI"},
				{"dob", "dob-Latn-ZZ"},
				{"doi", "doi-Deva-IN"},
				{"dop", "dop-Latn-ZZ"},
				{"dow", "dow-Latn-ZZ"},
				{"drh", "drh-Mong-CN"},
				{"dri", "dri-Latn-ZZ"},
				{"drs", "drs-Ethi-ZZ"},
				{"dsb", "dsb-Latn-DE"},
				{"dtm", "dtm-Latn-ML"},
				{"dtp", "dtp-Latn-MY"},
				{"dts", "dts-Latn-ZZ"},
				{"dty", "dty-Deva-NP"},
				{"dua", "dua-Latn-CM"},
				{"duc", "duc-Latn-ZZ"},
				{"dud", "dud-Latn-ZZ"},
				{"dug", "dug-Latn-ZZ"},
				{"dv", "dv-Thaa-MV"},
				{"dva", "dva-Latn-ZZ"},
				{"dww", "dww-Latn-ZZ"},
				{"dyo", "dyo-Latn-SN"},
				{"dyu", "dyu-Latn-BF"},
				{"dz", "dz-Tibt-BT"},
				{"dzg", "dzg-Latn-ZZ"},
				{"ebu", "ebu-Latn-KE"},
				{"ee", "ee-Latn-GH"},
				{"efi", "efi-Latn-NG"},
				{"egl", "egl-Latn-IT"},
				{"egy", "egy-Egyp-EG"},
				{"eka", "eka-Latn-ZZ"},
				{"eky", "eky-Kali-MM"},
				{"el", "el-Grek-GR"},
				{"ema", "ema-Latn-ZZ"},
				{"emi", "emi-Latn-ZZ"},
				{"en", "en-Latn-US"},
				{"en-gb-oed", "en-GB-oxendict"},
				{"enn", "enn-Latn-ZZ"},
				{"enq", "enq-Latn-ZZ"},
				{"en-shaw", "en-Shaw-GB"},
				{"eo", "eo-Latn-001"},
				{"eri", "eri-Latn-ZZ"},
				{"es", "es-Latn-ES"},
				{"esg", "esg-Gonm-IN"},
				{"esu", "esu-Latn-US"},
				{"et", "et-Latn-EE"},
				{"etr", "etr-Latn-ZZ"},
				{"ett", "ett-Ital-IT"},
				{"etu", "etu-Latn-ZZ"},
				{"etx", "etx-Latn-ZZ"},
				{"eu", "eu-Latn-ES"},
				{"ewo", "ewo-Latn-CM"},
				{"ext", "ext-Latn-ES"},
				{"eza", "eza-Latn-ZZ"},
				{"fa", "fa-Arab-IR"},
				{"faa", "faa-Latn-ZZ"},
				{"fab", "fab-Latn-ZZ"},
				{"fag", "fag-Latn-ZZ"},
				{"fai", "fai-Latn-ZZ"},
				{"fan", "fan-Latn-GQ"},
				{"ff", "ff-Latn-SN"},
				{"ff-adlm", "ff-Adlm-GN"},
				{"ffi", "ffi-Latn-ZZ"},
				{"ffm", "ffm-Latn-ML"},
				{"fi", "fi-Latn-FI"},
				{"fia", "fia-Arab-SD"},
				{"fil", "fil-Latn-PH"},
				{"fit", "fit-Latn-SE"},
				{"fj", "fj-Latn-FJ"},
				{"flr", "flr-Latn-ZZ"},
				{"fmp", "fmp-Latn-ZZ"},
				{"fo", "fo-Latn-FO"},
				{"fod", "fod-Latn-ZZ"},
				{"fon", "fon-Latn-BJ"},
				{"for", "for-Latn-ZZ"},
				{"fpe", "fpe-Latn-ZZ"},
				{"fqs", "fqs-Latn-ZZ"},
				{"fr", "fr-Latn-FR"},
				{"frc", "frc-Latn-US"},
				{"frp", "frp-Latn-FR"},
				{"frr", "frr-Latn-DE"},
				{"frs", "frs-Latn-DE"},
				{"fub", "fub-Arab-CM"},
				{"fud", "fud-Latn-WF"},
				{"fue", "fue-Latn-ZZ"},
				{"fuf", "fuf-Latn-GN"},
				{"fuh", "fuh-Latn-ZZ"},
				{"fuq", "fuq-Latn-NE"},
				{"fur", "fur-Latn-IT"},
				{"fuv", "fuv-Latn-NG"},
				{"fuy", "fuy-Latn-ZZ"},
				{"fvr", "fvr-Latn-SD"},
				{"fy", "fy-Latn-NL"},
				{"ga", "ga-Latn-IE"},
				{"gaa", "gaa-Latn-GH"},
				{"gaf", "gaf-Latn-ZZ"},
				{"gag", "gag-Latn-MD"},
				{"gah", "gah-Latn-ZZ"},
				{"gaj", "gaj-Latn-ZZ"},
				{"gam", "gam-Latn-ZZ"},
				{"gan", "gan-Hans-CN"},
				{"gaw", "gaw-Latn-ZZ"},
				{"gay", "gay-Latn-ID"},
				{"gba", "gba-Latn-ZZ"},
				{"gbf", "gbf-Latn-ZZ"},
				{"gbm", "gbm-Deva-IN"},
				{"gby", "gby-Latn-ZZ"},
				{"gbz", "gbz-Arab-IR"},
				{"gcr", "gcr-Latn-GF"},
				{"gd", "gd-Latn-GB"},
				{"gde", "gde-Latn-ZZ"},
				{"gdn", "gdn-Latn-ZZ"},
				{"gdr", "gdr-Latn-ZZ"},
				{"geb", "geb-Latn-ZZ"},
				{"gej", "gej-Latn-ZZ"},
				{"gel", "gel-Latn-ZZ"},
				{"gez", "gez-Ethi-ET"},
				{"gfk", "gfk-Latn-ZZ"},
				{"ggn", "ggn-Deva-NP"},
				{"ghs", "ghs-Latn-ZZ"},
				{"gil", "gil-Latn-KI"},
				{"gim", "gim-Latn-ZZ"},
				{"gjk", "gjk-Arab-PK"},
				{"gjn", "gjn-Latn-ZZ"},
				{"gju", "gju-Arab-PK"},
				{"gkn", "gkn-Latn-ZZ"},
				{"gkp", "gkp-Latn-ZZ"},
				{"gl", "gl-Latn-ES"},
				{"glk", "glk-Arab-IR"},
				{"gmm", "gmm-Latn-ZZ"},
				{"gmv", "gmv-Ethi-ZZ"},
				{"gn", "gn-Latn-PY"},
				{"gnd", "gnd-Latn-ZZ"},
				{"gng", "gng-Latn-ZZ"},
				{"god", "god-Latn-ZZ"},
				{"gof", "gof-Ethi-ZZ"},
				{"goi", "goi-Latn-ZZ"},
				{"gom", "gom-Deva-IN"},
				{"gon", "gon-Telu-IN"},
				{"gor", "gor-Latn-ID"},
				{"gos", "gos-Latn-NL"},
				{"got", "got-Goth-UA"},
				{"grb", "grb-Latn-ZZ"},
				{"grc", "grc-Cprt-CY"},
				{"grc-linb", "grc-Linb-GR"},
				{"grt", "grt-Beng-IN"},
				{"grw", "grw-Latn-ZZ"},
				{"gsw", "gsw-Latn-CH"},
				{"gu", "gu-Gujr-IN"},
				{"gub", "gub-Latn-BR"},
				{"guc", "guc-Latn-CO"},
				{"gud", "gud-Latn-ZZ"},
				{"gur", "gur-Latn-GH"},
				{"guw", "guw-Latn-ZZ"},
				{"gux", "gux-Latn-ZZ"},
				{"guz", "guz-Latn-KE"},
				{"gv", "gv-Latn-IM"},
				{"gvf", "gvf-Latn-ZZ"},
				{"gvr", "gvr-Deva-NP"},
				{"gvs", "gvs-Latn-ZZ"},
				{"gwc", "gwc-Arab-ZZ"},
				{"gwi", "gwi-Latn-CA"},
				{"gwt", "gwt-Arab-ZZ"},
				{"gyi", "gyi-Latn-ZZ"},
				{"ha", "ha-Latn-NG"},
				{"ha-cm", "ha-Arab-CM"},
				{"hag", "hag-Latn-ZZ"},
				{"hak", "hak-Hans-CN"},
				{"ham", "ham-Latn-ZZ"},
				{"ha-sd", "ha-Arab-SD"},
				{"haw", "haw-Latn-US"},
				{"haz", "haz-Arab-AF"},
				{"hbb", "hbb-Latn-ZZ"},
				{"hdy", "hdy-Ethi-ZZ"},
				{"he", "he-Hebr-IL"},
				{"hhy", "hhy-Latn-ZZ"},
				{"hi", "hi-Deva-IN"},
				{"hia", "hia-Latn-ZZ"},
				{"hif", "hif-Latn-FJ"},
				{"hig", "hig-Latn-ZZ"},
				{"hih", "hih-Latn-ZZ"},
				{"hil", "hil-Latn-PH"},
				{"hla", "hla-Latn-ZZ"},
				{"hlu", "hlu-Hluw-TR"},
				{"hmd", "hmd-Plrd-CN"},
				{"hmt", "hmt-Latn-ZZ"},
				{"hnd", "hnd-Arab-PK"},
				{"hne", "hne-Deva-IN"},
				{"hnj", "hnj-Hmnp-US"},
				{"hnn", "hnn-Latn-PH"},
				{"hno", "hno-Arab-PK"},
				{"ho", "ho-Latn-PG"},
				{"hoc", "hoc-Deva-IN"},
				{"hoj", "hoj-Deva-IN"},
				{"hot", "hot-Latn-ZZ"},
				{"hr", "hr-Latn-HR"},
				{"hsb", "hsb-Latn-DE"},
				{"hsn", "hsn-Hans-CN"},
				{"ht", "ht-Latn-HT"},
				{"hu", "hu-Latn-HU"},
				{"hui", "hui-Latn-ZZ"},
				{"hy", "hy-Armn-AM"},
				{"hz", "hz-Latn-NA"},
				{"ia", "ia-Latn-001"},
				{"i-ami", "ami"},// grandfathered: not in ISO 639
				{"ian", "ian-Latn-ZZ"},
				{"iar", "iar-Latn-ZZ"},
				{"iba", "iba-Latn-MY"},
				{"ibb", "ibb-Latn-NG"},
				{"i-bnn", "bnn"},// grandfathered: not in ISO 639
				{"iby", "iby-Latn-ZZ"},
				{"ica", "ica-Latn-ZZ"},
				{"ich", "ich-Latn-ZZ"},
				{"id", "id-Latn-ID"},
				{"idd", "idd-Latn-ZZ"},
				{"i-default", "en-Latn-US"},// grandfathered
				{"idi", "idi-Latn-ZZ"},
				{"idu", "idu-Latn-ZZ"},
				{"ife", "ife-Latn-TG"},
				{"ig", "ig-Latn-NG"},
				{"igb", "igb-Latn-ZZ"},
				{"ige", "ige-Latn-ZZ"},
				{"i-hak", "hak-Hans-CN"},// grandfathered
				{"ii", "ii-Yiii-CN"},
				{"ijj", "ijj-Latn-ZZ"},
				{"ik", "ik-Latn-US"},
				{"ikk", "ikk-Latn-ZZ"},
				{"i-klingon", "tlh"},// grandfathered: not in ISO 639
				{"ikt", "ikt-Latn-CA"},
				{"ikw", "ikw-Latn-ZZ"},
				{"ikx", "ikx-Latn-ZZ"},
				{"ilo", "ilo-Latn-PH"},
				{"i-lux", "lb-Latn-LU"},// grandfathered
				{"imo", "imo-Latn-ZZ"},
				{"in", "in-Latn-ID"},
				{"i-navajo", "nv-Latn-US"},// grandfathered
				{"inh", "inh-Cyrl-RU"},
				{"io", "io-Latn-001"},
				{"iou", "iou-Latn-ZZ"},
				{"i-pwn", "pwn"},// grandfathered: not in ISO 639
				{"iri", "iri-Latn-ZZ"},
				{"is", "is-Latn-IS"},
				{"it", "it-Latn-IT"},
				{"i-tao", "tao"},// grandfathered: not in ISO 639
				{"i-tay", "tay"},// grandfathered: not in ISO 639
				{"i-tsu", "tsu"},// grandfathered: not in ISO 639
				{"iu", "iu-Cans-CA"},
				{"iw", "iw-Hebr-IL"},
				{"iwm", "iwm-Latn-ZZ"},
				{"iws", "iws-Latn-ZZ"},
				{"izh", "izh-Latn-RU"},
				{"izi", "izi-Latn-ZZ"},
				{"ja", "ja-Jpan-JP"},
				{"jab", "jab-Latn-ZZ"},
				{"jam", "jam-Latn-JM"},
				{"jar", "jar-Latn-ZZ"},
				{"jbo", "jbo-Latn-001"},
				{"jbu", "jbu-Latn-ZZ"},
				{"jen", "jen-Latn-ZZ"},
				{"jgk", "jgk-Latn-ZZ"},
				{"jgo", "jgo-Latn-CM"},
				{"ji", "ji-Hebr-UA"},
				{"jib", "jib-Latn-ZZ"},
				{"jmc", "jmc-Latn-TZ"},
				{"jml", "jml-Deva-NP"},
				{"jra", "jra-Latn-ZZ"},
				{"jut", "jut-Latn-DK"},
				{"jv", "jv-Latn-ID"},
				{"jw", "jw-Latn-ID"},
				{"ka", "ka-Geor-GE"},
				{"kaa", "kaa-Cyrl-UZ"},
				{"kab", "kab-Latn-DZ"},
				{"kac", "kac-Latn-MM"},
				{"kad", "kad-Latn-ZZ"},
				{"kai", "kai-Latn-ZZ"},
				{"kaj", "kaj-Latn-NG"},
				{"kam", "kam-Latn-KE"},
				{"kao", "kao-Latn-ML"},
				{"kbd", "kbd-Cyrl-RU"},
				{"kbm", "kbm-Latn-ZZ"},
				{"kbp", "kbp-Latn-ZZ"},
				{"kbq", "kbq-Latn-ZZ"},
				{"kbx", "kbx-Latn-ZZ"},
				{"kby", "kby-Arab-NE"},
				{"kcg", "kcg-Latn-NG"},
				{"kck", "kck-Latn-ZW"},
				{"kcl", "kcl-Latn-ZZ"},
				{"kct", "kct-Latn-ZZ"},
				{"kde", "kde-Latn-TZ"},
				{"kdh", "kdh-Latn-TG"},
				{"kdl", "kdl-Latn-ZZ"},
				{"kdt", "kdt-Thai-TH"},
				{"kea", "kea-Latn-CV"},
				{"ken", "ken-Latn-CM"},
				{"kez", "kez-Latn-ZZ"},
				{"kfo", "kfo-Latn-CI"},
				{"kfr", "kfr-Deva-IN"},
				{"kfy", "kfy-Deva-IN"},
				{"kg", "kg-Latn-CD"},
				{"kge", "kge-Latn-ID"},
				{"kgf", "kgf-Latn-ZZ"},
				{"kgp", "kgp-Latn-BR"},
				{"kha", "kha-Latn-IN"},
				{"khb", "khb-Talu-CN"},
				{"khn", "khn-Deva-IN"},
				{"khq", "khq-Latn-ML"},
				{"khs", "khs-Latn-ZZ"},
				{"kht", "kht-Mymr-IN"},
				{"khw", "khw-Arab-PK"},
				{"khz", "khz-Latn-ZZ"},
				{"ki", "ki-Latn-KE"},
				{"kij", "kij-Latn-ZZ"},
				{"kiu", "kiu-Latn-TR"},
				{"kiw", "kiw-Latn-ZZ"},
				{"kj", "kj-Latn-NA"},
				{"kjd", "kjd-Latn-ZZ"},
				{"kjg", "kjg-Laoo-LA"},
				{"kjs", "kjs-Latn-ZZ"},
				{"kjy", "kjy-Latn-ZZ"},
				{"kk", "kk-Cyrl-KZ"},
				{"kk-af", "kk-Arab-AF"},
				{"kk-arab", "kk-Arab-CN"},
				{"kkc", "kkc-Latn-ZZ"},
				{"kk-cn", "kk-Arab-CN"},
				{"kk-ir", "kk-Arab-IR"},
				{"kkj", "kkj-Latn-CM"},
				{"kk-mn", "kk-Arab-MN"},
				{"kl", "kl-Latn-GL"},
				{"kln", "kln-Latn-KE"},
				{"klq", "klq-Latn-ZZ"},
				{"klt", "klt-Latn-ZZ"},
				{"klx", "klx-Latn-ZZ"},
				{"km", "km-Khmr-KH"},
				{"kmb", "kmb-Latn-AO"},
				{"kmh", "kmh-Latn-ZZ"},
				{"kmo", "kmo-Latn-ZZ"},
				{"kms", "kms-Latn-ZZ"},
				{"kmu", "kmu-Latn-ZZ"},
				{"kmw", "kmw-Latn-ZZ"},
				{"kn", "kn-Knda-IN"},
				{"knf", "knf-Latn-GW"},
				{"knp", "knp-Latn-ZZ"},
				{"ko", "ko-Kore-KR"},
				{"koi", "koi-Cyrl-RU"},
				{"kok", "kok-Deva-IN"},
				{"kol", "kol-Latn-ZZ"},
				{"kos", "kos-Latn-FM"},
				{"koz", "koz-Latn-ZZ"},
				{"kpe", "kpe-Latn-LR"},
				{"kpf", "kpf-Latn-ZZ"},
				{"kpo", "kpo-Latn-ZZ"},
				{"kpr", "kpr-Latn-ZZ"},
				{"kpx", "kpx-Latn-ZZ"},
				{"kqb", "kqb-Latn-ZZ"},
				{"kqf", "kqf-Latn-ZZ"},
				{"kqs", "kqs-Latn-ZZ"},
				{"kqy", "kqy-Ethi-ZZ"},
				{"kr", "kr-Latn-ZZ"},
				{"krc", "krc-Cyrl-RU"},
				{"kri", "kri-Latn-SL"},
				{"krj", "krj-Latn-PH"},
				{"krl", "krl-Latn-RU"},
				{"krs", "krs-Latn-ZZ"},
				{"kru", "kru-Deva-IN"},
				{"ks", "ks-Arab-IN"},
				{"ksb", "ksb-Latn-TZ"},
				{"ksd", "ksd-Latn-ZZ"},
				{"ksf", "ksf-Latn-CM"},
				{"ksh", "ksh-Latn-DE"},
				{"ksj", "ksj-Latn-ZZ"},
				{"ksr", "ksr-Latn-ZZ"},
				{"ktb", "ktb-Ethi-ZZ"},
				{"ktm", "ktm-Latn-ZZ"},
				{"kto", "kto-Latn-ZZ"},
				{"ktr", "ktr-Latn-MY"},
				{"ku", "ku-Latn-TR"},
				{"ku-arab", "ku-Arab-IQ"},
				{"kub", "kub-Latn-ZZ"},
				{"kud", "kud-Latn-ZZ"},
				{"kue", "kue-Latn-ZZ"},
				{"kuj", "kuj-Latn-ZZ"},
				{"ku-lb", "ku-Arab-LB"},
				{"kum", "kum-Cyrl-RU"},
				{"kun", "kun-Latn-ZZ"},
				{"kup", "kup-Latn-ZZ"},
				{"kus", "kus-Latn-ZZ"},
				{"ku-yezi", "ku-Yezi-GE"},
				{"kv", "kv-Cyrl-RU"},
				{"kvg", "kvg-Latn-ZZ"},
				{"kvr", "kvr-Latn-ID"},
				{"kvx", "kvx-Arab-PK"},
				{"kw", "kw-Latn-GB"},
				{"kwj", "kwj-Latn-ZZ"},
				{"kwo", "kwo-Latn-ZZ"},
				{"kwq", "kwq-Latn-ZZ"},
				{"kxa", "kxa-Latn-ZZ"},
				{"kxc", "kxc-Ethi-ZZ"},
				{"kxe", "kxe-Latn-ZZ"},
				{"kxl", "kxl-Deva-IN"},
				{"kxm", "kxm-Thai-TH"},
				{"kxp", "kxp-Arab-PK"},
				{"kxw", "kxw-Latn-ZZ"},
				{"kxz", "kxz-Latn-ZZ"},
				{"ky", "ky-Cyrl-KG"},
				{"ky-arab", "ky-Arab-CN"},
				{"ky-cn", "ky-Arab-CN"},
				{"kye", "kye-Latn-ZZ"},
				{"ky-latn", "ky-Latn-TR"},
				{"ky-tr", "ky-Latn-TR"},
				{"kyx", "kyx-Latn-ZZ"},
				{"kzh", "kzh-Arab-ZZ"},
				{"kzj", "kzj-Latn-MY"},
				{"kzr", "kzr-Latn-ZZ"},
				{"kzt", "kzt-Latn-MY"},
				{"la", "la-Latn-VA"},
				{"lab", "lab-Lina-GR"},
				{"lad", "lad-Hebr-IL"},
				{"lag", "lag-Latn-TZ"},
				{"lah", "lah-Arab-PK"},
				{"laj", "laj-Latn-UG"},
				{"las", "las-Latn-ZZ"},
				{"lb", "lb-Latn-LU"},
				{"lbe", "lbe-Cyrl-RU"},
				{"lbu", "lbu-Latn-ZZ"},
				{"lbw", "lbw-Latn-ID"},
				{"lcm", "lcm-Latn-ZZ"},
				{"lcp", "lcp-Thai-CN"},
				{"ldb", "ldb-Latn-ZZ"},
				{"led", "led-Latn-ZZ"},
				{"lee", "lee-Latn-ZZ"},
				{"lem", "lem-Latn-ZZ"},
				{"lep", "lep-Lepc-IN"},
				{"leq", "leq-Latn-ZZ"},
				{"leu", "leu-Latn-ZZ"},
				{"lez", "lez-Cyrl-RU"},
				{"lg", "lg-Latn-UG"},
				{"lgg", "lgg-Latn-ZZ"},
				{"li", "li-Latn-NL"},
				{"lia", "lia-Latn-ZZ"},
				{"lid", "lid-Latn-ZZ"},
				{"lif", "lif-Deva-NP"},
				{"lif-limb", "lif-Limb-IN"},
				{"lig", "lig-Latn-ZZ"},
				{"lih", "lih-Latn-ZZ"},
				{"lij", "lij-Latn-IT"},
				{"lis", "lis-Lisu-CN"},
				{"ljp", "ljp-Latn-ID"},
				{"lki", "lki-Arab-IR"},
				{"lkt", "lkt-Latn-US"},
				{"lle", "lle-Latn-ZZ"},
				{"lln", "lln-Latn-ZZ"},
				{"lmn", "lmn-Telu-IN"},
				{"lmo", "lmo-Latn-IT"},
				{"lmp", "lmp-Latn-ZZ"},
				{"ln", "ln-Latn-CD"},
				{"lns", "lns-Latn-ZZ"},
				{"lnu", "lnu-Latn-ZZ"},
				{"lo", "lo-Laoo-LA"},
				{"loj", "loj-Latn-ZZ"},
				{"lok", "lok-Latn-ZZ"},
				{"lol", "lol-Latn-CD"},
				{"lor", "lor-Latn-ZZ"},
				{"los", "los-Latn-ZZ"},
				{"loz", "loz-Latn-ZM"},
				{"lrc", "lrc-Arab-IR"},
				{"lt", "lt-Latn-LT"},
				{"ltg", "ltg-Latn-LV"},
				{"lu", "lu-Latn-CD"},
				{"lua", "lua-Latn-CD"},
				{"luo", "luo-Latn-KE"},
				{"luy", "luy-Latn-KE"},
				{"luz", "luz-Arab-IR"},
				{"lv", "lv-Latn-LV"},
				{"lwl", "lwl-Thai-TH"},
				{"lzh", "lzh-Hans-CN"},
				{"lzz", "lzz-Latn-TR"},
				{"mad", "mad-Latn-ID"},
				{"maf", "maf-Latn-CM"},
				{"mag", "mag-Deva-IN"},
				{"mai", "mai-Deva-IN"},
				{"mak", "mak-Latn-ID"},
				{"man", "man-Latn-GM"},
				{"man-gn", "man-Nkoo-GN"},
				{"man-nkoo", "man-Nkoo-GN"},
				{"mas", "mas-Latn-KE"},
				{"maw", "maw-Latn-ZZ"},
				{"maz", "maz-Latn-MX"},
				{"mbh", "mbh-Latn-ZZ"},
				{"mbo", "mbo-Latn-ZZ"},
				{"mbq", "mbq-Latn-ZZ"},
				{"mbu", "mbu-Latn-ZZ"},
				{"mbw", "mbw-Latn-ZZ"},
				{"mci", "mci-Latn-ZZ"},
				{"mcp", "mcp-Latn-ZZ"},
				{"mcq", "mcq-Latn-ZZ"},
				{"mcr", "mcr-Latn-ZZ"},
				{"mcu", "mcu-Latn-ZZ"},
				{"mda", "mda-Latn-ZZ"},
				{"mde", "mde-Arab-ZZ"},
				{"mdf", "mdf-Cyrl-RU"},
				{"mdh", "mdh-Latn-PH"},
				{"mdj", "mdj-Latn-ZZ"},
				{"mdr", "mdr-Latn-ID"},
				{"mdx", "mdx-Ethi-ZZ"},
				{"med", "med-Latn-ZZ"},
				{"mee", "mee-Latn-ZZ"},
				{"mek", "mek-Latn-ZZ"},
				{"men", "men-Latn-SL"},
				{"mer", "mer-Latn-KE"},
				{"met", "met-Latn-ZZ"},
				{"meu", "meu-Latn-ZZ"},
				{"mfa", "mfa-Arab-TH"},
				{"mfe", "mfe-Latn-MU"},
				{"mfn", "mfn-Latn-ZZ"},
				{"mfo", "mfo-Latn-ZZ"},
				{"mfq", "mfq-Latn-ZZ"},
				{"mg", "mg-Latn-MG"},
				{"mgh", "mgh-Latn-MZ"},
				{"mgl", "mgl-Latn-ZZ"},
				{"mgo", "mgo-Latn-CM"},
				{"mgp", "mgp-Deva-NP"},
				{"mgy", "mgy-Latn-TZ"},
				{"mh", "mh-Latn-MH"},
				{"mhi", "mhi-Latn-ZZ"},
				{"mhl", "mhl-Latn-ZZ"},
				{"mi", "mi-Latn-NZ"},
				{"mif", "mif-Latn-ZZ"},
				{"min", "min-Latn-ID"},
				{"miw", "miw-Latn-ZZ"},
				{"mk", "mk-Cyrl-MK"},
				{"mki", "mki-Arab-ZZ"},
				{"mkl", "mkl-Latn-ZZ"},
				{"mkp", "mkp-Latn-ZZ"},
				{"mkw", "mkw-Latn-ZZ"},
				{"ml", "ml-Mlym-IN"},
				{"mle", "mle-Latn-ZZ"},
				{"mlp", "mlp-Latn-ZZ"},
				{"mls", "mls-Latn-SD"},
				{"mmo", "mmo-Latn-ZZ"},
				{"mmu", "mmu-Latn-ZZ"},
				{"mmx", "mmx-Latn-ZZ"},
				{"mn", "mn-Cyrl-MN"},
				{"mna", "mna-Latn-ZZ"},
				{"mn-cn", "mn-Mong-CN"},
				{"mnf", "mnf-Latn-ZZ"},
				{"mni", "mni-Beng-IN"},
				{"mn-mong", "mn-Mong-CN"},
				{"mnw", "mnw-Mymr-MM"},
				{"mo", "mo-Latn-RO"},
				{"moa", "moa-Latn-ZZ"},
				{"moe", "moe-Latn-CA"},
				{"moh", "moh-Latn-CA"},
				{"mos", "mos-Latn-BF"},
				{"mox", "mox-Latn-ZZ"},
				{"mpp", "mpp-Latn-ZZ"},
				{"mps", "mps-Latn-ZZ"},
				{"mpt", "mpt-Latn-ZZ"},
				{"mpx", "mpx-Latn-ZZ"},
				{"mql", "mql-Latn-ZZ"},
				{"mr", "mr-Deva-IN"},
				{"mrd", "mrd-Deva-NP"},
				{"mrj", "mrj-Cyrl-RU"},
				{"mro", "mro-Mroo-BD"},
				{"ms", "ms-Latn-MY"},
				{"ms-cc", "ms-Arab-CC"},
				{"mt", "mt-Latn-MT"},
				{"mtc", "mtc-Latn-ZZ"},
				{"mtf", "mtf-Latn-ZZ"},
				{"mti", "mti-Latn-ZZ"},
				{"mtr", "mtr-Deva-IN"},
				{"mua", "mua-Latn-CM"},
				{"mur", "mur-Latn-ZZ"},
				{"mus", "mus-Latn-US"},
				{"mva", "mva-Latn-ZZ"},
				{"mvn", "mvn-Latn-ZZ"},
				{"mvy", "mvy-Arab-PK"},
				{"mwk", "mwk-Latn-ML"},
				{"mwr", "mwr-Deva-IN"},
				{"mwv", "mwv-Latn-ID"},
				{"mww", "mww-Hmnp-US"},
				{"mxc", "mxc-Latn-ZW"},
				{"mxm", "mxm-Latn-ZZ"},
				{"my", "my-Mymr-MM"},
				{"myk", "myk-Latn-ZZ"},
				{"mym", "mym-Ethi-ZZ"},
				{"myv", "myv-Cyrl-RU"},
				{"myw", "myw-Latn-ZZ"},
				{"myx", "myx-Latn-UG"},
				{"myz", "myz-Mand-IR"},
				{"mzk", "mzk-Latn-ZZ"},
				{"mzm", "mzm-Latn-ZZ"},
				{"mzn", "mzn-Arab-IR"},
				{"mzp", "mzp-Latn-ZZ"},
				{"mzw", "mzw-Latn-ZZ"},
				{"mzz", "mzz-Latn-ZZ"},
				{"na", "na-Latn-NR"},
				{"nac", "nac-Latn-ZZ"},
				{"naf", "naf-Latn-ZZ"},
				{"nak", "nak-Latn-ZZ"},
				{"nan", "nan-Hans-CN"},
				{"nap", "nap-Latn-IT"},
				{"naq", "naq-Latn-NA"},
				{"nas", "nas-Latn-ZZ"},
				{"nb", "nb-Latn-NO"},
				{"nca", "nca-Latn-ZZ"},
				{"nce", "nce-Latn-ZZ"},
				{"ncf", "ncf-Latn-ZZ"},
				{"nch", "nch-Latn-MX"},
				{"nco", "nco-Latn-ZZ"},
				{"ncu", "ncu-Latn-ZZ"},
				{"nd", "nd-Latn-ZW"},
				{"ndc", "ndc-Latn-MZ"},
				{"nds", "nds-Latn-DE"},
				{"ne", "ne-Deva-NP"},
				{"neb", "neb-Latn-ZZ"},
				{"new", "new-Deva-NP"},
				{"nex", "nex-Latn-ZZ"},
				{"nfr", "nfr-Latn-ZZ"},
				{"ng", "ng-Latn-NA"},
				{"nga", "nga-Latn-ZZ"},
				{"ngb", "ngb-Latn-ZZ"},
				{"ngl", "ngl-Latn-MZ"},
				{"nhb", "nhb-Latn-ZZ"},
				{"nhe", "nhe-Latn-MX"},
				{"nhw", "nhw-Latn-MX"},
				{"nif", "nif-Latn-ZZ"},
				{"nii", "nii-Latn-ZZ"},
				{"nij", "nij-Latn-ID"},
				{"nin", "nin-Latn-ZZ"},
				{"niu", "niu-Latn-NU"},
				{"niy", "niy-Latn-ZZ"},
				{"niz", "niz-Latn-ZZ"},
				{"njo", "njo-Latn-IN"},
				{"nkg", "nkg-Latn-ZZ"},
				{"nko", "nko-Latn-ZZ"},
				{"nl", "nl-Latn-NL"},
				{"nmg", "nmg-Latn-CM"},
				{"nmz", "nmz-Latn-ZZ"},
				{"nn", "nn-Latn-NO"},
				{"nnf", "nnf-Latn-ZZ"},
				{"nnh", "nnh-Latn-CM"},
				{"nnk", "nnk-Latn-ZZ"},
				{"nnm", "nnm-Latn-ZZ"},
				{"nnp", "nnp-Wcho-IN"},
				{"no", "no-Latn-NO"},
				{"no-bok", "nb-Latn-NO"},// grandfathered
				{"nod", "nod-Lana-TH"},
				{"noe", "noe-Deva-IN"},
				{"non", "non-Runr-SE"},
				{"no-nyn", "nn-Latn-NO"},// grandfathered
				{"nop", "nop-Latn-ZZ"},
				{"nou", "nou-Latn-ZZ"},
				{"nqo", "nqo-Nkoo-GN"},
				{"nr", "nr-Latn-ZA"},
				{"nrb", "nrb-Latn-ZZ"},
				{"nsk", "nsk-Cans-CA"},
				{"nsn", "nsn-Latn-ZZ"},
				{"nso", "nso-Latn-ZA"},
				{"nss", "nss-Latn-ZZ"},
				{"nst", "nst-Tnsa-IN"},
				{"ntm", "ntm-Latn-ZZ"},
				{"ntr", "ntr-Latn-ZZ"},
				{"nui", "nui-Latn-ZZ"},
				{"nup", "nup-Latn-ZZ"},
				{"nus", "nus-Latn-SS"},
				{"nuv", "nuv-Latn-ZZ"},
				{"nux", "nux-Latn-ZZ"},
				{"nv", "nv-Latn-US"},
				{"nwb", "nwb-Latn-ZZ"},
				{"nxq", "nxq-Latn-CN"},
				{"nxr", "nxr-Latn-ZZ"},
				{"ny", "ny-Latn-MW"},
				{"nym", "nym-Latn-TZ"},
				{"nyn", "nyn-Latn-UG"},
				{"nzi", "nzi-Latn-GH"},
				{"oc", "oc-Latn-FR"},
				{"ogc", "ogc-Latn-ZZ"},
				{"okr", "okr-Latn-ZZ"},
				{"okv", "okv-Latn-ZZ"},
				{"om", "om-Latn-ET"},
				{"ong", "ong-Latn-ZZ"},
				{"onn", "onn-Latn-ZZ"},
				{"ons", "ons-Latn-ZZ"},
				{"opm", "opm-Latn-ZZ"},
				{"or", "or-Orya-IN"},
				{"oro", "oro-Latn-ZZ"},
				{"oru", "oru-Arab-ZZ"},
				{"os", "os-Cyrl-GE"},
				{"osa", "osa-Osge-US"},
				{"ota", "ota-Arab-ZZ"},
				{"otk", "otk-Orkh-MN"},
				{"oui", "oui-Ougr-143"},
				{"ozm", "ozm-Latn-ZZ"},
				{"pa", "pa-Guru-IN"},
				{"pa-arab", "pa-Arab-PK"},
				{"pag", "pag-Latn-PH"},
				{"pal", "pal-Phli-IR"},
				{"pal-phlp", "pal-Phlp-CN"},
				{"pam", "pam-Latn-PH"},
				{"pap", "pap-Latn-AW"},
				{"pa-pk", "pa-Arab-PK"},
				{"pau", "pau-Latn-PW"},
				{"pbi", "pbi-Latn-ZZ"},
				{"pcd", "pcd-Latn-FR"},
				{"pcm", "pcm-Latn-NG"},
				{"pdc", "pdc-Latn-US"},
				{"pdt", "pdt-Latn-CA"},
				{"ped", "ped-Latn-ZZ"},
				{"peo", "peo-Xpeo-IR"},
				{"pex", "pex-Latn-ZZ"},
				{"pfl", "pfl-Latn-DE"},
				{"phl", "phl-Arab-ZZ"},
				{"phn", "phn-Phnx-LB"},
				{"pil", "pil-Latn-ZZ"},
				{"pip", "pip-Latn-ZZ"},
				{"pka", "pka-Brah-IN"},
				{"pko", "pko-Latn-KE"},
				{"pl", "pl-Latn-PL"},
				{"pla", "pla-Latn-ZZ"},
				{"pms", "pms-Latn-IT"},
				{"png", "png-Latn-ZZ"},
				{"pnn", "pnn-Latn-ZZ"},
				{"pnt", "pnt-Grek-GR"},
				{"pon", "pon-Latn-FM"},
				{"ppa", "ppa-Deva-IN"},
				{"ppo", "ppo-Latn-ZZ"},
				{"pra", "pra-Khar-PK"},
				{"prd", "prd-Arab-IR"},
				{"prg", "prg-Latn-001"},
				{"ps", "ps-Arab-AF"},
				{"pss", "pss-Latn-ZZ"},
				{"pt", "pt-Latn-BR"},
				{"ptp", "ptp-Latn-ZZ"},
				{"puu", "puu-Latn-GA"},
				{"pwa", "pwa-Latn-ZZ"},
				{"qu", "qu-Latn-PE"},
				{"quc", "quc-Latn-GT"},
				{"qug", "qug-Latn-EC"},
				{"rai", "rai-Latn-ZZ"},
				{"raj", "raj-Deva-IN"},
				{"rao", "rao-Latn-ZZ"},
				{"rcf", "rcf-Latn-RE"},
				{"rej", "rej-Latn-ID"},
				{"rel", "rel-Latn-ZZ"},
				{"res", "res-Latn-ZZ"},
				{"rgn", "rgn-Latn-IT"},
				{"rhg", "rhg-Rohg-MM"},
				{"ria", "ria-Latn-IN"},
				{"rif", "rif-Tfng-MA"},
				{"rif-nl", "rif-Latn-NL"},
				{"rjs", "rjs-Deva-NP"},
				{"rkt", "rkt-Beng-BD"},
				{"rm", "rm-Latn-CH"},
				{"rmf", "rmf-Latn-FI"},
				{"rmo", "rmo-Latn-CH"},
				{"rmt", "rmt-Arab-IR"},
				{"rmu", "rmu-Latn-SE"},
				{"rn", "rn-Latn-BI"},
				{"rna", "rna-Latn-ZZ"},
				{"rng", "rng-Latn-MZ"},
				{"ro", "ro-Latn-RO"},
				{"rob", "rob-Latn-ID"},
				{"rof", "rof-Latn-TZ"},
				{"roo", "roo-Latn-ZZ"},
				{"rro", "rro-Latn-ZZ"},
				{"rtm", "rtm-Latn-FJ"},
				{"ru", "ru-Cyrl-RU"},
				{"rue", "rue-Cyrl-UA"},
				{"rug", "rug-Latn-SB"},
				{"rw", "rw-Latn-RW"},
				{"rwk", "rwk-Latn-TZ"},
				{"rwo", "rwo-Latn-ZZ"},
				{"ryu", "ryu-Kana-JP"},
				{"sa", "sa-Deva-IN"},
				{"saf", "saf-Latn-GH"},
				{"sah", "sah-Cyrl-RU"},
				{"saq", "saq-Latn-KE"},
				{"sas", "sas-Latn-ID"},
				{"sat", "sat-Olck-IN"},
				{"sav", "sav-Latn-SN"},
				{"saz", "saz-Saur-IN"},
				{"sba", "sba-Latn-ZZ"},
				{"sbe", "sbe-Latn-ZZ"},
				{"sbp", "sbp-Latn-TZ"},
				{"sc", "sc-Latn-IT"},
				{"sck", "sck-Deva-IN"},
				{"scl", "scl-Arab-ZZ"},
				{"scn", "scn-Latn-IT"},
				{"sco", "sco-Latn-GB"},
				{"scs", "scs-Latn-CA"},
				{"sd", "sd-Arab-PK"},
				{"sdc", "sdc-Latn-IT"},
				{"sd-deva", "sd-Deva-IN"},
				{"sd-dind", "sd-Sind-IN"},
				{"sdh", "sdh-Arab-IR"},
				{"sd-khoj", "sd-Khoj-IN"},
				{"se", "se-Latn-NO"},
				{"sef", "sef-Latn-CI"},
				{"seh", "seh-Latn-MZ"},
				{"sei", "sei-Latn-MX"},
				{"ses", "ses-Latn-ML"},
				{"sg", "sg-Latn-CF"},
				{"sga", "sga-Ogam-IE"},
				{"sgn-be-fr", "fr-Sigw-BE"},// grandfathered
				{"sgn-be-nl", "nl-Sigw-BE"},// grandfathered
				{"sgn-ch-de", "de-Sigw-CH"},// grandfathered
				{"sgs", "sgs-Latn-LT"},
				{"sgw", "sgw-Ethi-ZZ"},
				{"sgz", "sgz-Latn-ZZ"},
				{"shi", "shi-Tfng-MA"},
				{"shk", "shk-Latn-ZZ"},
				{"shn", "shn-Mymr-MM"},
				{"shu", "shu-Arab-ZZ"},
				{"si", "si-Sinh-LK"},
				{"sid", "sid-Latn-ET"},
				{"sig", "sig-Latn-ZZ"},
				{"sil", "sil-Latn-ZZ"},
				{"sim", "sim-Latn-ZZ"},
				{"sjr", "sjr-Latn-ZZ"},
				{"sk", "sk-Latn-SK"},
				{"skc", "skc-Latn-ZZ"},
				{"skr", "skr-Arab-PK"},
				{"sks", "sks-Latn-ZZ"},
				{"sl", "sl-Latn-SI"},
				{"sld", "sld-Latn-ZZ"},
				{"sli", "sli-Latn-PL"},
				{"sll", "sll-Latn-ZZ"},
				{"sly", "sly-Latn-ID"},
				{"sm", "sm-Latn-WS"},
				{"sma", "sma-Latn-SE"},
				{"smj", "smj-Latn-SE"},
				{"smn", "smn-Latn-FI"},
				{"smp", "smp-Samr-IL"},
				{"smq", "smq-Latn-ZZ"},
				{"sms", "sms-Latn-FI"},
				{"sn", "sn-Latn-ZW"},
				{"snc", "snc-Latn-ZZ"},
				{"snk", "snk-Latn-ML"},
				{"snp", "snp-Latn-ZZ"},
				{"snx", "snx-Latn-ZZ"},
				{"sny", "sny-Latn-ZZ"},
				{"so", "so-Latn-SO"},
				{"sog", "sog-Sogd-UZ"},
				{"sok", "sok-Latn-ZZ"},
				{"soq", "soq-Latn-ZZ"},
				{"sou", "sou-Thai-TH"},
				{"soy", "soy-Latn-ZZ"},
				{"spd", "spd-Latn-ZZ"},
				{"spl", "spl-Latn-ZZ"},
				{"sps", "sps-Latn-ZZ"},
				{"sq", "sq-Latn-AL"},
				{"sr", "sr-Cyrl-RS"},
				{"srb", "srb-Sora-IN"},
				{"sr-me", "sr-Latn-ME"},
				{"srn", "srn-Latn-SR"},
				{"srr", "srr-Latn-SN"},
				{"sr-ro", "sr-Latn-RO"},
				{"sr-ru", "sr-Latn-RU"},
				{"sr-tr", "sr-Latn-TR"},
				{"srx", "srx-Deva-IN"},
				{"ss", "ss-Latn-ZA"},
				{"ssd", "ssd-Latn-ZZ"},
				{"ssg", "ssg-Latn-ZZ"},
				{"ssy", "ssy-Latn-ER"},
				{"st", "st-Latn-ZA"},
				{"stk", "stk-Latn-ZZ"},
				{"stq", "stq-Latn-DE"},
				{"su", "su-Latn-ID"},
				{"sua", "sua-Latn-ZZ"},
				{"sue", "sue-Latn-ZZ"},
				{"suk", "suk-Latn-TZ"},
				{"sur", "sur-Latn-ZZ"},
				{"sus", "sus-Latn-GN"},
				{"sv", "sv-Latn-SE"},
				{"sw", "sw-Latn-TZ"},
				{"swb", "swb-Arab-YT"},
				{"swc", "swc-Latn-CD"},
				{"swg", "swg-Latn-DE"},
				{"swp", "swp-Latn-ZZ"},
				{"swv", "swv-Deva-IN"},
				{"sxn", "sxn-Latn-ID"},
				{"sxw", "sxw-Latn-ZZ"},
				{"syl", "syl-Beng-BD"},
				{"syr", "syr-Syrc-IQ"},
				{"szl", "szl-Latn-PL"},
				{"ta", "ta-Taml-IN"},
				{"taj", "taj-Deva-NP"},
				{"tal", "tal-Latn-ZZ"},
				{"tan", "tan-Latn-ZZ"},
				{"taq", "taq-Latn-ZZ"},
				{"tbc", "tbc-Latn-ZZ"},
				{"tbd", "tbd-Latn-ZZ"},
				{"tbf", "tbf-Latn-ZZ"},
				{"tbg", "tbg-Latn-ZZ"},
				{"tbo", "tbo-Latn-ZZ"},
				{"tbw", "tbw-Latn-PH"},
				{"tbz", "tbz-Latn-ZZ"},
				{"tci", "tci-Latn-ZZ"},
				{"tcy", "tcy-Knda-IN"},
				{"tdd", "tdd-Tale-CN"},
				{"tdg", "tdg-Deva-NP"},
				{"tdh", "tdh-Deva-NP"},
				{"tdu", "tdu-Latn-MY"},
				{"te", "te-Telu-IN"},
				{"ted", "ted-Latn-ZZ"},
				{"tem", "tem-Latn-SL"},
				{"teo", "teo-Latn-UG"},
				{"tet", "tet-Latn-TL"},
				{"tfi", "tfi-Latn-ZZ"},
				{"tg", "tg-Cyrl-TJ"},
				{"tg-arab", "tg-Arab-PK"},
				{"tgc", "tgc-Latn-ZZ"},
				{"tgo", "tgo-Latn-ZZ"},
				{"tg-pk", "tg-Arab-PK"},
				{"tgu", "tgu-Latn-ZZ"},
				{"th", "th-Thai-TH"},
				{"thl", "thl-Deva-NP"},
				{"thq", "thq-Deva-NP"},
				{"thr", "thr-Deva-NP"},
				{"ti", "ti-Ethi-ET"},
				{"tif", "tif-Latn-ZZ"},
				{"tig", "tig-Ethi-ER"},
				{"tik", "tik-Latn-ZZ"},
				{"tim", "tim-Latn-ZZ"},
				{"tio", "tio-Latn-ZZ"},
				{"tiv", "tiv-Latn-NG"},
				{"tk", "tk-Latn-TM"},
				{"tkl", "tkl-Latn-TK"},
				{"tkr", "tkr-Latn-AZ"},
				{"tkt", "tkt-Deva-NP"},
				{"tl", "tl-Latn-PH"},
				{"tlf", "tlf-Latn-ZZ"},
				{"tlx", "tlx-Latn-ZZ"},
				{"tly", "tly-Latn-AZ"},
				{"tmh", "tmh-Latn-NE"},
				{"tmy", "tmy-Latn-ZZ"},
				{"tn", "tn-Latn-ZA"},
				{"tnh", "tnh-Latn-ZZ"},
				{"to", "to-Latn-TO"},
				{"tof", "tof-Latn-ZZ"},
				{"tog", "tog-Latn-MW"},
				{"toq", "toq-Latn-ZZ"},
				{"tpi", "tpi-Latn-PG"},
				{"tpm", "tpm-Latn-ZZ"},
				{"tpz", "tpz-Latn-ZZ"},
				{"tqo", "tqo-Latn-ZZ"},
				{"tr", "tr-Latn-TR"},
				{"tru", "tru-Latn-TR"},
				{"trv", "trv-Latn-TW"},
				{"trw", "trw-Arab-PK"},
				{"ts", "ts-Latn-ZA"},
				{"tsd", "tsd-Grek-GR"},
				{"tsf", "tsf-Deva-NP"},
				{"tsg", "tsg-Latn-PH"},
				{"tsj", "tsj-Tibt-BT"},
				{"tsw", "tsw-Latn-ZZ"},
				{"tt", "tt-Cyrl-RU"},
				{"ttd", "ttd-Latn-ZZ"},
				{"tte", "tte-Latn-ZZ"},
				{"ttj", "ttj-Latn-UG"},
				{"ttr", "ttr-Latn-ZZ"},
				{"tts", "tts-Thai-TH"},
				{"ttt", "ttt-Latn-AZ"},
				{"tuh", "tuh-Latn-ZZ"},
				{"tul", "tul-Latn-ZZ"},
				{"tum", "tum-Latn-MW"},
				{"tuq", "tuq-Latn-ZZ"},
				{"tvd", "tvd-Latn-ZZ"},
				{"tvl", "tvl-Latn-TV"},
				{"tvu", "tvu-Latn-ZZ"},
				{"twh", "twh-Latn-ZZ"},
				{"twq", "twq-Latn-NE"},
				{"txg", "txg-Tang-CN"},
				{"txo", "txo-Toto-IN"},
				{"ty", "ty-Latn-PF"},
				{"tya", "tya-Latn-ZZ"},
				{"tyv", "tyv-Cyrl-RU"},
				{"tzm", "tzm-Latn-MA"},
				{"ubu", "ubu-Latn-ZZ"},
				{"udi", "udi-Aghb-RU"},
				{"udm", "udm-Cyrl-RU"},
				{"ug", "ug-Arab-CN"},
				{"uga", "uga-Ugar-SY"},
				{"ug-cyrl", "ug-Cyrl-KZ"},
				{"ug-kz", "ug-Cyrl-KZ"},
				{"ug-mn", "ug-Cyrl-MN"},
				{"uk", "uk-Cyrl-UA"},
				{"uli", "uli-Latn-FM"},
				{"umb", "umb-Latn-AO"},
				{"und", "en-Latn-US"},
				{"und-002", "en-Latn-NG"},
				{"und-003", "en-Latn-US"},
				{"und-005", "pt-Latn-BR"},
				{"und-009", "en-Latn-AU"},
				{"und-011", "en-Latn-NG"},
				{"und-013", "es-Latn-MX"},
				{"und-014", "sw-Latn-TZ"},
				{"und-015", "ar-Arab-EG"},
				{"und-017", "sw-Latn-CD"},
				{"und-018", "en-Latn-ZA"},
				{"und-019", "en-Latn-US"},
				{"und-021", "en-Latn-US"},
				{"und-029", "es-Latn-CU"},
				{"und-030", "zh-Hans-CN"},
				{"und-034", "hi-Deva-IN"},
				{"und-035", "id-Latn-ID"},
				{"und-039", "it-Latn-IT"},
				{"und-053", "en-Latn-AU"},
				{"und-054", "en-Latn-PG"},
				{"und-057", "en-Latn-GU"},
				{"und-061", "sm-Latn-WS"},
				{"und-142", "zh-Hans-CN"},
				{"und-143", "uz-Latn-UZ"},
				{"und-145", "ar-Arab-SA"},
				{"und-150", "ru-Cyrl-RU"},
				{"und-151", "ru-Cyrl-RU"},
				{"und-154", "en-Latn-GB"},
				{"und-155", "de-Latn-DE"},
				{"und-202", "en-Latn-NG"},
				{"und-419", "es-Latn-419"},
				{"und-ad", "ca-Latn-AD"},
				{"und-adlm", "ff-Adlm-GN"},
				{"und-ae", "ar-Arab-AE"},
				{"und-af", "fa-Arab-AF"},
				{"und-aghb", "udi-Aghb-RU"},
				{"und-ahom", "aho-Ahom-IN"},
				{"und-al", "sq-Latn-AL"},
				{"und-am", "hy-Armn-AM"},
				{"und-ao", "pt-Latn-AO"},
				{"und-aq", "und-Latn-AQ"},
				{"und-ar", "es-Latn-AR"},
				{"und-arab", "ar-Arab-EG"},
				{"und-arab-cc", "ms-Arab-CC"},
				{"und-arab-cn", "ug-Arab-CN"},
				{"und-arab-gb", "ks-Arab-GB"},
				{"und-arab-id", "ms-Arab-ID"},
				{"und-arab-in", "ur-Arab-IN"},
				{"und-arab-kh", "cja-Arab-KH"},
				{"und-arab-mm", "rhg-Arab-MM"},
				{"und-arab-mn", "kk-Arab-MN"},
				{"und-arab-mu", "ur-Arab-MU"},
				{"und-arab-ng", "ha-Arab-NG"},
				{"und-arab-pk", "ur-Arab-PK"},
				{"und-arab-tg", "apd-Arab-TG"},
				{"und-arab-th", "mfa-Arab-TH"},
				{"und-arab-tj", "fa-Arab-TJ"},
				{"und-arab-tr", "az-Arab-TR"},
				{"und-arab-yt", "swb-Arab-YT"},
				{"und-armi", "arc-Armi-IR"},
				{"und-armn", "hy-Armn-AM"},
				{"und-as", "sm-Latn-AS"},
				{"und-at", "de-Latn-AT"},
				{"und-avst", "ae-Avst-IR"},
				{"und-aw", "nl-Latn-AW"},
				{"und-ax", "sv-Latn-AX"},
				{"und-az", "az-Latn-AZ"},
				{"und-ba", "bs-Latn-BA"},
				{"und-bali", "ban-Bali-ID"},
				{"und-bamu", "bax-Bamu-CM"},
				{"und-bass", "bsq-Bass-LR"},
				{"und-batk", "bbc-Batk-ID"},
				{"und-bd", "bn-Beng-BD"},
				{"und-be", "nl-Latn-BE"},
				{"und-beng", "bn-Beng-BD"},
				{"und-bf", "fr-Latn-BF"},
				{"und-bg", "bg-Cyrl-BG"},
				{"und-bh", "ar-Arab-BH"},
				{"und-bhks", "sa-Bhks-IN"},
				{"und-bi", "rn-Latn-BI"},
				{"und-bj", "fr-Latn-BJ"},
				{"und-bl", "fr-Latn-BL"},
				{"und-bn", "ms-Latn-BN"},
				{"und-bo", "es-Latn-BO"},
				{"und-bopo", "zh-Bopo-TW"},
				{"und-bq", "pap-Latn-BQ"},
				{"und-br", "pt-Latn-BR"},
				{"und-brah", "pka-Brah-IN"},
				{"und-brai", "fr-Brai-FR"},
				{"und-bt", "dz-Tibt-BT"},
				{"und-bugi", "bug-Bugi-ID"},
				{"und-buhd", "bku-Buhd-PH"},
				{"und-bv", "und-Latn-BV"},
				{"und-by", "be-Cyrl-BY"},
				{"und-cakm", "ccp-Cakm-BD"},
				{"und-cans", "cr-Cans-CA"},
				{"und-cari", "xcr-Cari-TR"},
				{"und-cd", "sw-Latn-CD"},
				{"und-cf", "fr-Latn-CF"},
				{"und-cg", "fr-Latn-CG"},
				{"und-ch", "de-Latn-CH"},
				{"und-cham", "cjm-Cham-VN"},
				{"und-cher", "chr-Cher-US"},
				{"und-chrs", "xco-Chrs-UZ"},
				{"und-ci", "fr-Latn-CI"},
				{"und-cl", "es-Latn-CL"},
				{"und-cm", "fr-Latn-CM"},
				{"und-cn", "zh-Hans-CN"},
				{"und-co", "es-Latn-CO"},
				{"und-copt", "cop-Copt-EG"},
				{"und-cp", "und-Latn-CP"},
				{"und-cpmn", "und-Cpmn-CY"},
				{"und-cprt", "grc-Cprt-CY"},
				{"und-cr", "es-Latn-CR"},
				{"und-cu", "es-Latn-CU"},
				{"und-cv", "pt-Latn-CV"},
				{"und-cw", "pap-Latn-CW"},
				{"und-cy", "el-Grek-CY"},
				{"und-cyrl", "ru-Cyrl-RU"},
				{"und-cyrl-al", "mk-Cyrl-AL"},
				{"und-cyrl-ba", "sr-Cyrl-BA"},
				{"und-cyrl-ge", "os-Cyrl-GE"},
				{"und-cyrl-gr", "mk-Cyrl-GR"},
				{"und-cyrl-md", "uk-Cyrl-MD"},
				{"und-cyrl-ro", "bg-Cyrl-RO"},
				{"und-cyrl-sk", "uk-Cyrl-SK"},
				{"und-cyrl-tr", "kbd-Cyrl-TR"},
				{"und-cyrl-xk", "sr-Cyrl-XK"},
				{"und-cz", "cs-Latn-CZ"},
				{"und-de", "de-Latn-DE"},
				{"und-deva", "hi-Deva-IN"},
				{"und-deva-bt", "ne-Deva-BT"},
				{"und-deva-fj", "hif-Deva-FJ"},
				{"und-deva-mu", "bho-Deva-MU"},
				{"und-deva-pk", "btv-Deva-PK"},
				{"und-diak", "dv-Diak-MV"},
				{"und-dj", "aa-Latn-DJ"},
				{"und-dk", "da-Latn-DK"},
				{"und-do", "es-Latn-DO"},
				{"und-dogr", "doi-Dogr-IN"},
				{"und-dupl", "fr-Dupl-FR"},
				{"und-dz", "ar-Arab-DZ"},
				{"und-ea", "es-Latn-EA"},
				{"und-ec", "es-Latn-EC"},
				{"und-ee", "et-Latn-EE"},
				{"und-eg", "ar-Arab-EG"},
				{"und-egyp", "egy-Egyp-EG"},
				{"und-eh", "ar-Arab-EH"},
				{"und-elba", "sq-Elba-AL"},
				{"und-elym", "arc-Elym-IR"},
				{"und-er", "ti-Ethi-ER"},
				{"und-es", "es-Latn-ES"},
				{"und-et", "am-Ethi-ET"},
				{"und-ethi", "am-Ethi-ET"},
				{"und-eu", "en-Latn-IE"},
				{"und-ez", "de-Latn-EZ"},
				{"und-fi", "fi-Latn-FI"},
				{"und-fo", "fo-Latn-FO"},
				{"und-fr", "fr-Latn-FR"},
				{"und-ga", "fr-Latn-GA"},
				{"und-ge", "ka-Geor-GE"},
				{"und-geor", "ka-Geor-GE"},
				{"und-gf", "fr-Latn-GF"},
				{"und-gh", "ak-Latn-GH"},
				{"und-gl", "kl-Latn-GL"},
				{"und-glag", "cu-Glag-BG"},
				{"und-gn", "fr-Latn-GN"},
				{"und-gong", "wsg-Gong-IN"},
				{"und-gonm", "esg-Gonm-IN"},
				{"und-goth", "got-Goth-UA"},
				{"und-gp", "fr-Latn-GP"},
				{"und-gq", "es-Latn-GQ"},
				{"und-gr", "el-Grek-GR"},
				{"und-gran", "sa-Gran-IN"},
				{"und-grek", "el-Grek-GR"},
				{"und-grek-tr", "bgx-Grek-TR"},
				{"und-gs", "und-Latn-GS"},
				{"und-gt", "es-Latn-GT"},
				{"und-gujr", "gu-Gujr-IN"},
				{"und-guru", "pa-Guru-IN"},
				{"und-gw", "pt-Latn-GW"},
				{"und-hanb", "zh-Hanb-TW"},
				{"und-hang", "ko-Hang-KR"},
				{"und-hani", "zh-Hani-CN"},
				{"und-hano", "hnn-Hano-PH"},
				{"und-hans", "zh-Hans-CN"},
				{"und-hant", "zh-Hant-TW"},
				{"und-hebr", "he-Hebr-IL"},
				{"und-hebr-ca", "yi-Hebr-CA"},
				{"und-hebr-gb", "yi-Hebr-GB"},
				{"und-hebr-se", "yi-Hebr-SE"},
				{"und-hebr-ua", "yi-Hebr-UA"},
				{"und-hebr-us", "yi-Hebr-US"},
				{"und-hira", "ja-Hira-JP"},
				{"und-hk", "zh-Hant-HK"},
				{"und-hluw", "hlu-Hluw-TR"},
				{"und-hm", "und-Latn-HM"},
				{"und-hmng", "hnj-Hmng-LA"},
				{"und-hmnp", "hnj-Hmnp-US"},
				{"und-hn", "es-Latn-HN"},
				{"und-hr", "hr-Latn-HR"},
				{"und-ht", "ht-Latn-HT"},
				{"und-hu", "hu-Latn-HU"},
				{"und-hung", "hu-Hung-HU"},
				{"und-ic", "es-Latn-IC"},
				{"und-id", "id-Latn-ID"},
				{"und-il", "he-Hebr-IL"},
				{"und-in", "hi-Deva-IN"},
				{"und-iq", "ar-Arab-IQ"},
				{"und-ir", "fa-Arab-IR"},
				{"und-is", "is-Latn-IS"},
				{"und-it", "it-Latn-IT"},
				{"und-ital", "ett-Ital-IT"},
				{"und-jamo", "ko-Jamo-KR"},
				{"und-java", "jv-Java-ID"},
				{"und-jo", "ar-Arab-JO"},
				{"und-jp", "ja-Jpan-JP"},
				{"und-jpan", "ja-Jpan-JP"},
				{"und-kali", "eky-Kali-MM"},
				{"und-kana", "ja-Kana-JP"},
				{"und-ke", "sw-Latn-KE"},
				{"und-kg", "ky-Cyrl-KG"},
				{"und-kh", "km-Khmr-KH"},
				{"und-khar", "pra-Khar-PK"},
				{"und-khmr", "km-Khmr-KH"},
				{"und-khoj", "sd-Khoj-IN"},
				{"und-kits", "zkt-Kits-CN"},
				{"und-km", "ar-Arab-KM"},
				{"und-knda", "kn-Knda-IN"},
				{"und-kore", "ko-Kore-KR"},
				{"und-kp", "ko-Kore-KP"},
				{"und-kr", "ko-Kore-KR"},
				{"und-kthi", "bho-Kthi-IN"},
				{"und-kw", "ar-Arab-KW"},
				{"und-kz", "ru-Cyrl-KZ"},
				{"und-la", "lo-Laoo-LA"},
				{"und-lana", "nod-Lana-TH"},
				{"und-laoo", "lo-Laoo-LA"},
				{"und-latn-af", "tk-Latn-AF"},
				{"und-latn-am", "ku-Latn-AM"},
				{"und-latn-cn", "za-Latn-CN"},
				{"und-latn-cy", "tr-Latn-CY"},
				{"und-latn-dz", "fr-Latn-DZ"},
				{"und-latn-et", "en-Latn-ET"},
				{"und-latn-ge", "ku-Latn-GE"},
				{"und-latn-ir", "tk-Latn-IR"},
				{"und-latn-km", "fr-Latn-KM"},
				{"und-latn-ma", "fr-Latn-MA"},
				{"und-latn-mk", "sq-Latn-MK"},
				{"und-latn-mm", "kac-Latn-MM"},
				{"und-latn-mo", "pt-Latn-MO"},
				{"und-latn-mr", "fr-Latn-MR"},
				{"und-latn-ru", "krl-Latn-RU"},
				{"und-latn-sy", "fr-Latn-SY"},
				{"und-latn-tn", "fr-Latn-TN"},
				{"und-latn-tw", "trv-Latn-TW"},
				{"und-latn-ua", "pl-Latn-UA"},
				{"und-lb", "ar-Arab-LB"},
				{"und-lepc", "lep-Lepc-IN"},
				{"und-li", "de-Latn-LI"},
				{"und-limb", "lif-Limb-IN"},
				{"und-lina", "lab-Lina-GR"},
				{"und-linb", "grc-Linb-GR"},
				{"und-lisu", "lis-Lisu-CN"},
				{"und-lk", "si-Sinh-LK"},
				{"und-ls", "st-Latn-LS"},
				{"und-lt", "lt-Latn-LT"},
				{"und-lu", "fr-Latn-LU"},
				{"und-lv", "lv-Latn-LV"},
				{"und-ly", "ar-Arab-LY"},
				{"und-lyci", "xlc-Lyci-TR"},
				{"und-lydi", "xld-Lydi-TR"},
				{"und-ma", "ar-Arab-MA"},
				{"und-mahj", "hi-Mahj-IN"},
				{"und-maka", "mak-Maka-ID"},
				{"und-mand", "myz-Mand-IR"},
				{"und-mani", "xmn-Mani-CN"},
				{"und-marc", "bo-Marc-CN"},
				{"und-mc", "fr-Latn-MC"},
				{"und-md", "ro-Latn-MD"},
				{"und-me", "sr-Latn-ME"},
				{"und-medf", "dmf-Medf-NG"},
				{"und-mend", "men-Mend-SL"},
				{"und-merc", "xmr-Merc-SD"},
				{"und-mero", "xmr-Mero-SD"},
				{"und-mf", "fr-Latn-MF"},
				{"und-mg", "mg-Latn-MG"},
				{"und-mk", "mk-Cyrl-MK"},
				{"und-ml", "bm-Latn-ML"},
				{"und-mlym", "ml-Mlym-IN"},
				{"und-mm", "my-Mymr-MM"},
				{"und-mn", "mn-Cyrl-MN"},
				{"und-mo", "zh-Hant-MO"},
				{"und-modi", "mr-Modi-IN"},
				{"und-mong", "mn-Mong-CN"},
				{"und-mq", "fr-Latn-MQ"},
				{"und-mr", "ar-Arab-MR"},
				{"und-mroo", "mro-Mroo-BD"},
				{"und-mt", "mt-Latn-MT"},
				{"und-mtei", "mni-Mtei-IN"},
				{"und-mu", "mfe-Latn-MU"},
				{"und-mult", "skr-Mult-PK"},
				{"und-mv", "dv-Thaa-MV"},
				{"und-mx", "es-Latn-MX"},
				{"und-my", "ms-Latn-MY"},
				{"und-mymr", "my-Mymr-MM"},
				{"und-mymr-in", "kht-Mymr-IN"},
				{"und-mymr-th", "mnw-Mymr-TH"},
				{"und-mz", "pt-Latn-MZ"},
				{"und-na", "af-Latn-NA"},
				{"und-nand", "sa-Nand-IN"},
				{"und-narb", "xna-Narb-SA"},
				{"und-nbat", "arc-Nbat-JO"},
				{"und-nc", "fr-Latn-NC"},
				{"und-ne", "ha-Latn-NE"},
				{"und-newa", "new-Newa-NP"},
				{"und-ni", "es-Latn-NI"},
				{"und-nkoo", "man-Nkoo-GN"},
				{"und-nl", "nl-Latn-NL"},
				{"und-no", "nb-Latn-NO"},
				{"und-np", "ne-Deva-NP"},
				{"und-nshu", "zhx-Nshu-CN"},
				{"und-ogam", "sga-Ogam-IE"},
				{"und-olck", "sat-Olck-IN"},
				{"und-om", "ar-Arab-OM"},
				{"und-orkh", "otk-Orkh-MN"},
				{"und-orya", "or-Orya-IN"},
				{"und-osge", "osa-Osge-US"},
				{"und-osma", "so-Osma-SO"},
				{"und-ougr", "oui-Ougr-143"},
				{"und-pa", "es-Latn-PA"},
				{"und-palm", "arc-Palm-SY"},
				{"und-pauc", "ctd-Pauc-MM"},
				{"und-pe", "es-Latn-PE"},
				{"und-perm", "kv-Perm-RU"},
				{"und-pf", "fr-Latn-PF"},
				{"und-pg", "tpi-Latn-PG"},
				{"und-ph", "fil-Latn-PH"},
				{"und-phag", "lzh-Phag-CN"},
				{"und-phli", "pal-Phli-IR"},
				{"und-phlp", "pal-Phlp-CN"},
				{"und-phnx", "phn-Phnx-LB"},
				{"und-pk", "ur-Arab-PK"},
				{"und-pl", "pl-Latn-PL"},
				{"und-plrd", "hmd-Plrd-CN"},
				{"und-pm", "fr-Latn-PM"},
				{"und-pr", "es-Latn-PR"},
				{"und-prti", "xpr-Prti-IR"},
				{"und-ps", "ar-Arab-PS"},
				{"und-pt", "pt-Latn-PT"},
				{"und-pw", "pau-Latn-PW"},
				{"und-py", "gn-Latn-PY"},
				{"und-qa", "ar-Arab-QA"},
				{"und-qo", "en-Latn-DG"},
				{"und-re", "fr-Latn-RE"},
				{"und-rjng", "rej-Rjng-ID"},
				{"und-ro", "ro-Latn-RO"},
				{"und-rohg", "rhg-Rohg-MM"},
				{"und-rs", "sr-Cyrl-RS"},
				{"und-ru", "ru-Cyrl-RU"},
				{"und-runr", "non-Runr-SE"},
				{"und-rw", "rw-Latn-RW"},
				{"und-sa", "ar-Arab-SA"},
				{"und-samr", "smp-Samr-IL"},
				{"und-sarb", "xsa-Sarb-YE"},
				{"und-saur", "saz-Saur-IN"},
				{"und-sc", "fr-Latn-SC"},
				{"und-sd", "ar-Arab-SD"},
				{"und-se", "sv-Latn-SE"},
				{"und-sgnw", "ase-Sgnw-US"},
				{"und-shaw", "en-Shaw-GB"},
				{"und-shrd", "sa-Shrd-IN"},
				{"und-si", "sl-Latn-SI"},
				{"und-sidd", "sa-Sidd-IN"},
				{"und-sind", "sd-Sind-IN"},
				{"und-sinh", "si-Sinh-LK"},
				{"und-sj", "nb-Latn-SJ"},
				{"und-sk", "sk-Latn-SK"},
				{"und-sm", "it-Latn-SM"},
				{"und-sn", "fr-Latn-SN"},
				{"und-so", "so-Latn-SO"},
				{"und-sogd", "sog-Sogd-UZ"},
				{"und-sogo", "sog-Sogo-UZ"},
				{"und-sora", "srb-Sora-IN"},
				{"und-soyo", "cmg-Soyo-MN"},
				{"und-sr", "nl-Latn-SR"},
				{"und-st", "pt-Latn-ST"},
				{"und-sund", "su-Sund-ID"},
				{"und-sv", "es-Latn-SV"},
				{"und-sy", "ar-Arab-SY"},
				{"und-sylo", "syl-Sylo-BD"},
				{"und-syrc", "syr-Syrc-IQ"},
				{"und-tagb", "tbw-Tagb-PH"},
				{"und-takr", "doi-Takr-IN"},
				{"und-tale", "tdd-Tale-CN"},
				{"und-talu", "khb-Talu-CN"},
				{"und-taml", "ta-Taml-IN"},
				{"und-tang", "txg-Tang-CN"},
				{"und-tavt", "blt-Tavt-VN"},
				{"und-td", "fr-Latn-TD"},
				{"und-telu", "te-Telu-IN"},
				{"und-tf", "fr-Latn-TF"},
				{"und-tfng", "zgh-Tfng-MA"},
				{"und-tg", "fr-Latn-TG"},
				{"und-tglg", "fil-Tglg-PH"},
				{"und-th", "th-Thai-TH"},
				{"und-thaa", "dv-Thaa-MV"},
				{"und-thai", "th-Thai-TH"},
				{"und-thai-cn", "lcp-Thai-CN"},
				{"und-thai-kh", "kdt-Thai-KH"},
				{"und-thai-la", "kdt-Thai-LA"},
				{"und-tibt", "bo-Tibt-CN"},
				{"und-tirh", "mai-Tirh-IN"},
				{"und-tj", "tg-Cyrl-TJ"},
				{"und-tk", "tkl-Latn-TK"},
				{"und-tl", "pt-Latn-TL"},
				{"und-tm", "tk-Latn-TM"},
				{"und-tn", "ar-Arab-TN"},
				{"und-tnsa", "nst-Tnsa-IN"},
				{"und-to", "to-Latn-TO"},
				{"und-toto", "txo-Toto-IN"},
				{"und-tr", "tr-Latn-TR"},
				{"und-tv", "tvl-Latn-TV"},
				{"und-tw", "zh-Hant-TW"},
				{"und-tz", "sw-Latn-TZ"},
				{"und-ua", "uk-Cyrl-UA"},
				{"und-ug", "sw-Latn-UG"},
				{"und-ugar", "uga-Ugar-SY"},
				{"und-uy", "es-Latn-UY"},
				{"und-uz", "uz-Latn-UZ"},
				{"und-va", "it-Latn-VA"},
				{"und-vaii", "vai-Vaii-LR"},
				{"und-ve", "es-Latn-VE"},
				{"und-vith", "sq-Vith-AL"},
				{"und-vn", "vi-Latn-VN"},
				{"und-vu", "bi-Latn-VU"},
				{"und-wara", "hoc-Wara-IN"},
				{"und-wcho", "nnp-Wcho-IN"},
				{"und-wf", "fr-Latn-WF"},
				{"und-ws", "sm-Latn-WS"},
				{"und-xk", "sq-Latn-XK"},
				{"und-xpeo", "peo-Xpeo-IR"},
				{"und-xsux", "akk-Xsux-IQ"},
				{"und-ye", "ar-Arab-YE"},
				{"und-yezi", "ku-Yezi-GE"},
				{"und-yiii", "ii-Yiii-CN"},
				{"und-yt", "fr-Latn-YT"},
				{"und-zanb", "cmg-Zanb-MN"},
				{"und-zw", "sn-Latn-ZW"},
				{"unr", "unr-Beng-IN"},
				{"unr-deva", "unr-Deva-NP"},
				{"unr-np", "unr-Deva-NP"},
				{"unx", "unx-Beng-IN"},
				{"uok", "uok-Latn-ZZ"},
				{"ur", "ur-Arab-PK"},
				{"uri", "uri-Latn-ZZ"},
				{"urt", "urt-Latn-ZZ"},
				{"urw", "urw-Latn-ZZ"},
				{"usa", "usa-Latn-ZZ"},
				{"uth", "uth-Latn-ZZ"},
				{"utr", "utr-Latn-ZZ"},
				{"uvh", "uvh-Latn-ZZ"},
				{"uvl", "uvl-Latn-ZZ"},
				{"uz", "uz-Latn-UZ"},
				{"uz-af", "uz-Arab-AF"},
				{"uz-arab", "uz-Arab-AF"},
				{"uz-cn", "uz-Cyrl-CN"},
				{"vag", "vag-Latn-ZZ"},
				{"vai", "vai-Vaii-LR"},
				{"van", "van-Latn-ZZ"},
				{"ve", "ve-Latn-ZA"},
				{"vec", "vec-Latn-IT"},
				{"vep", "vep-Latn-RU"},
				{"vi", "vi-Latn-VN"},
				{"vic", "vic-Latn-SX"},
				{"viv", "viv-Latn-ZZ"},
				{"vls", "vls-Latn-BE"},
				{"vmf", "vmf-Latn-DE"},
				{"vmw", "vmw-Latn-MZ"},
				{"vo", "vo-Latn-001"},
				{"vot", "vot-Latn-RU"},
				{"vro", "vro-Latn-EE"},
				{"vun", "vun-Latn-TZ"},
				{"vut", "vut-Latn-ZZ"},
				{"wa", "wa-Latn-BE"},
				{"wae", "wae-Latn-CH"},
				{"waj", "waj-Latn-ZZ"},
				{"wal", "wal-Ethi-ET"},
				{"wan", "wan-Latn-ZZ"},
				{"war", "war-Latn-PH"},
				{"wbp", "wbp-Latn-AU"},
				{"wbq", "wbq-Telu-IN"},
				{"wbr", "wbr-Deva-IN"},
				{"wci", "wci-Latn-ZZ"},
				{"wer", "wer-Latn-ZZ"},
				{"wgi", "wgi-Latn-ZZ"},
				{"whg", "whg-Latn-ZZ"},
				{"wib", "wib-Latn-ZZ"},
				{"wiu", "wiu-Latn-ZZ"},
				{"wiv", "wiv-Latn-ZZ"},
				{"wja", "wja-Latn-ZZ"},
				{"wji", "wji-Latn-ZZ"},
				{"wls", "wls-Latn-WF"},
				{"wmo", "wmo-Latn-ZZ"},
				{"wnc", "wnc-Latn-ZZ"},
				{"wni", "wni-Arab-KM"},
				{"wnu", "wnu-Latn-ZZ"},
				{"wo", "wo-Latn-SN"},
				{"wob", "wob-Latn-ZZ"},
				{"wos", "wos-Latn-ZZ"},
				{"wrs", "wrs-Latn-ZZ"},
				{"wsg", "wsg-Gong-IN"},
				{"wsk", "wsk-Latn-ZZ"},
				{"wtm", "wtm-Deva-IN"},
				{"wuu", "wuu-Hans-CN"},
				{"wuv", "wuv-Latn-ZZ"},
				{"wwa", "wwa-Latn-ZZ"},
				{"xav", "xav-Latn-BR"},
				{"xbi", "xbi-Latn-ZZ"},
				{"xco", "xco-Chrs-UZ"},
				{"xcr", "xcr-Cari-TR"},
				{"xes", "xes-Latn-ZZ"},
				{"xh", "xh-Latn-ZA"},
				{"xla", "xla-Latn-ZZ"},
				{"xlc", "xlc-Lyci-TR"},
				{"xld", "xld-Lydi-TR"},
				{"xmf", "xmf-Geor-GE"},
				{"xmn", "xmn-Mani-CN"},
				{"xmr", "xmr-Merc-SD"},
				{"xna", "xna-Narb-SA"},
				{"xnr", "xnr-Deva-IN"},
				{"xog", "xog-Latn-UG"},
				{"xon", "xon-Latn-ZZ"},
				{"xpr", "xpr-Prti-IR"},
				{"xrb", "xrb-Latn-ZZ"},
				{"xsa", "xsa-Sarb-YE"},
				{"xsi", "xsi-Latn-ZZ"},
				{"xsm", "xsm-Latn-ZZ"},
				{"xsr", "xsr-Deva-NP"},
				{"xwe", "xwe-Latn-ZZ"},
				{"yam", "yam-Latn-ZZ"},
				{"yao", "yao-Latn-MZ"},
				{"yap", "yap-Latn-FM"},
				{"yas", "yas-Latn-ZZ"},
				{"yat", "yat-Latn-ZZ"},
				{"yav", "yav-Latn-CM"},
				{"yay", "yay-Latn-ZZ"},
				{"yaz", "yaz-Latn-ZZ"},
				{"yba", "yba-Latn-ZZ"},
				{"ybb", "ybb-Latn-CM"},
				{"yby", "yby-Latn-ZZ"},
				{"yer", "yer-Latn-ZZ"},
				{"ygr", "ygr-Latn-ZZ"},
				{"ygw", "ygw-Latn-ZZ"},
				{"yi", "yi-Hebr-001"},
				{"yko", "yko-Latn-ZZ"},
				{"yle", "yle-Latn-ZZ"},
				{"ylg", "ylg-Latn-ZZ"},
				{"yll", "yll-Latn-ZZ"},
				{"yml", "yml-Latn-ZZ"},
				{"yo", "yo-Latn-NG"},
				{"yon", "yon-Latn-ZZ"},
				{"yrb", "yrb-Latn-ZZ"},
				{"yre", "yre-Latn-ZZ"},
				{"yrl", "yrl-Latn-BR"},
				{"yss", "yss-Latn-ZZ"},
				{"yua", "yua-Latn-MX"},
				{"yue", "yue-Hant-HK"},
				{"yue-cn", "yue-Hans-CN"},
				{"yue-hans", "yue-Hans-CN"},
				{"yuj", "yuj-Latn-ZZ"},
				{"yut", "yut-Latn-ZZ"},
				{"yuw", "yuw-Latn-ZZ"},
				{"za", "za-Latn-CN"},
				{"zag", "zag-Latn-SD"},
				{"zdj", "zdj-Arab-KM"},
				{"zea", "zea-Latn-NL"},
				{"zgh", "zgh-Tfng-MA"},
				{"zh", "zh-Hans-CN"},
				{"zh-au", "zh-Hant-AU"},
				{"zh-bn", "zh-Hant-BN"},
				{"zh-bopo", "zh-Bopo-TW"},
				{"zh-gb", "zh-Hant-GB"},
				{"zh-gf", "zh-Hant-GF"},
				{"zh-guoyu", "cmn"},        // grandfathered
				{"zh-hakka", "hak-Hans-CN"},// grandfathered
				{"zh-hanb", "zh-Hanb-TW"},
				{"zh-hant", "zh-Hant-TW"},
				{"zh-hk", "zh-Hant-HK"},
				{"zh-id", "zh-Hant-ID"},
				{"zh-min-nan", "nan-Hans-CN"},// grandfathered
				{"zh-mo", "zh-Hant-MO"},
				{"zh-pa", "zh-Hant-PA"},
				{"zh-pf", "zh-Hant-PF"},
				{"zh-ph", "zh-Hant-PH"},
				{"zh-sr", "zh-Hant-SR"},
				{"zh-th", "zh-Hant-TH"},
				{"zh-tw", "zh-Hant-TW"},
				{"zh-us", "zh-Hant-US"},
				{"zh-vn", "zh-Hant-VN"},
				{"zhx", "zhx-Nshu-CN"},
				{"zh-xiang", "hsn-Hans-CN"},// grandfathered
				{"zia", "zia-Latn-ZZ"},
				{"zkt", "zkt-Kits-CN"},
				{"zlm", "zlm-Latn-TG"},
				{"zmi", "zmi-Latn-MY"},
				{"zne", "zne-Latn-ZZ"},
				{"zu", "zu-Latn-ZA"},
				{"zza", "zza-Latn-TR"}};

		std::array<IETFLanguageTag::tag_info, std::ranges::size(data)> result{};
		std::ranges::copy(data, result.data());

		return result;
	}

	constexpr auto ietf_language_tag_tag_info_database      = make_ietf_language_tag_tag_info_database();
	constexpr auto ietf_language_tag_tag_info_database_size = std::ranges::size(ietf_language_tag_tag_info_database);

	[[nodiscard]] constexpr auto ietf_language_tag_tag_info_mapping_from_to_to(const std::basic_string_view<IETFLanguageTag::element_type> string) noexcept -> std::optional<std::basic_string_view<IETFLanguageTag::element_type>>
	{
		const auto lower_string = infrastructure::to_lower(string);

		if (const auto it = std::ranges::lower_bound(
					ietf_language_tag_tag_info_database,
					lower_string,
					std::ranges::less{},
					&IETFLanguageTag::tag_info::from);
			it != ietf_language_tag_tag_info_database.end() and it->from == lower_string) { return it->to; }

		return std::nullopt;
	}

	export
	{
		constexpr auto IETFLanguageTag::parse(const std::basic_string_view<element_type> string) noexcept -> std::optional<IETFLanguageTag>
		{
			std::optional<language_type> language{};
			std::optional<region_type>   region{};
			std::optional<script_type>   script{};

			// fixme:
			// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
			//	          with
			//	          [
			//	              _Ty=std::tuple<std::basic_string_view<char,std::char_traits<char>>>
			//	          ]
			//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: maybe "std::tuple_size<std::tuple<_Types...>>"
			//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: or        "std::tuple_size<std::tuple<_Types...>>"
			//
			// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
			//	          with
			//	          [
			//	              _Ty=std::tuple<char>
			//	          ]
			//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: maybe "std::tuple_size<std::tuple<_Types...>>"
			//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(643): note: or        "std::tuple_size<std::tuple<_Types...>>"
			// auto all_part =
			// 		string |
			// 		// '_'
			// 		std::views::split('_')// std::views::split(std::basic_string_view{"_"});
			// 		|
			// 		// '-'
			// 		std::views::transform(
			// 				[](auto&& view)
			// 				{
			// 					return view | std::views::split('-');// std::views::split(std::basic_string_view{"-"});
			// 				}) |
			// 		// join
			// 		std::views::join;
			std::vector<std::basic_string_view<element_type>> all_part{};
			for (std::basic_string_view<element_type>::size_type index = 0, pos = string.find_first_of("_-", index);; pos = string.find_first_of("_-", index))
			{
				if (pos == std::basic_string_view<element_type>::npos) { pos = string.size(); }

				all_part.emplace_back(string.data() + index, pos - index);
				index = pos + 1;

				if (pos == string.size()) { break; }
			}

			// https://en.wikipedia.org/wiki/IETF_language_tag#Syntax_of_language_tags
			for (
				std::basic_string_view<element_type> extension_string = {};
				const auto                           each_view: all_part
			)
			{
				// const auto each_string_view = std::ranges::to<std::basic_string_view<element_type>>(each_view);
				const auto each_string_view = std::basic_string_view{each_view};

				if (not extension_string.empty())
				{
					// Once inside the extensions portion of a language tag you can no longer determine validity based on just the element size.
				}
				else if (not language)
				{
					if (each_string_view == "*")
					{
						// wildcard
						language = language_type{0};
					}
					else
					{
						// if (each_string_view.size() == 2 or each_string_view.size() == 3)
						if (language = language_type::parse(each_string_view);
							not language)
						{
							// pass failed
							return std::nullopt;
						}
					}
				}
				else
				{
					if (not region and not script and each_string_view.size() == 3 and infrastructure::is_alpha(each_string_view))
					{
						// Up to 3 optional 3 letter extended language codes.
						// Ignore these for backward compatibility.
					}
					else if (not region and not script and each_string_view.size() == 4 and infrastructure::is_alpha(each_string_view))
					{
						// An optional script sub-tag, based on a four-letter script code from ISO 15924 (usually written in Title Case).
						script = script_type::parse(each_string_view);
					}
					else if (
						not region and
						(
							(each_string_view.size() == 2 and infrastructure::is_alpha(each_string_view))
							//
							or
							//
							(each_string_view.size() == 3 and infrastructure::is_digit(each_string_view))
						)
					)
					{
						// An optional region subtag based on a two-letter country code from ISO 3166-1 alpha-2 (usually written in upper case),
						// or a three-digit code from UN M.49 for geographical regions.
						region = region_type::parse(each_string_view);
					}
					else if ((each_string_view.size() >= 5 and each_string_view.size() <= 8) or (each_string_view.size() == 4 and infrastructure::is_digit(each_string_view.front())))
					{
						// A variant has 5 to 8 letters or a 4 digit + letters code.
					}
					else if (each_string_view.size() == 1)
					{
						// Start of an extension. We do not differentiate with private-use indicator.
						extension_string = each_string_view;
					}
					else
					{
						// pass failed
						return std::nullopt;
					}
				}
			}

			GAL_PROMETHEUS_DEBUG_ASSUME(language.has_value());
			return IETFLanguageTag{*language, region.value_or(region_type{}), script.value_or(script_type{})};
		}

		constexpr auto IETFLanguageTag::expand() const noexcept -> IETFLanguageTag
		{
			auto result{*this};

			if (not region_.empty() and not script_.empty()) { return result; }

			if (const auto to = ietf_language_tag_tag_info_mapping_from_to_to(result.language_.code());
				to.has_value())
			{
				const auto tag = parse(*to);

				GAL_PROMETHEUS_DEBUG_ASSUME(tag.has_value());

				if (result.region_.empty()) { result.region_ = tag->region_; }

				if (result.script_.empty()) { result.script_ = tag->script_; }
			}

			if (not result.region_.empty() and not result.script_.empty()) { return result; }

			if (const auto to = ietf_language_tag_tag_info_mapping_from_to_to("und-" + result.region_.code2());
				to.has_value())
			{
				const auto tag = parse(*to);

				GAL_PROMETHEUS_DEBUG_ASSUME(tag.has_value());

				if (result.script_.empty()) { result.script_ = tag->script_; }
			}

			return result;
		}

		constexpr auto IETFLanguageTag::shrink() const noexcept -> IETFLanguageTag
		{
			if (const auto vs = canonical_variants();
				vs.empty()) { return IETFLanguageTag{*this}; }
			else { return vs.back(); }
		}
	}
}

export namespace std
{
	// fixme:
	// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\format(2748): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
	//	          with
	//	          [
	//	              _Ty=const std::to_chars_result
	//	          ]
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(604): note: maybe "std::tuple_size<_Ty>"
	//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(604): note: or        "std::tuple_size<_Ty>"
	template<>
	struct formatter<gal::prometheus::i18n::IETFLanguageTag>//: formatter<std::basic_string<gal::prometheus::i18n::IETFLanguageTag::element_type>>
	{
		using char_type = gal::prometheus::i18n::IETFLanguageTag::element_type;

		constexpr auto parse(basic_format_parse_context<char_type>& parse_context) const noexcept -> basic_format_parse_context<char_type>::iterator { return parse_context.end(); }

		/*constexpr*/
		auto format(const gal::prometheus::i18n::IETFLanguageTag& tag, format_context& context) const -> format_context::iterator
		{
			// return formatter<std::basic_string<char_type>>::format(tag.operator std::basic_string<char_type>(), context);
			auto       it     = context.out();
			const auto string = tag.operator std::basic_string<char_type>();
			std::ranges::for_each(
					string,
					[&it](const char c) { it = c; });
			return it;
		}
	};
}
