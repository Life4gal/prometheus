// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <regex>
#include <ranges>
#include <type_traits>
#include <charconv>
#include <cstdint>
#include <algorithm>

#if CLP_USE_EXPECTED
#include <expected>
#else
#include <optional>
#endif

#include <prometheus/macro.hpp>

#if CLP_USE_EXPECTED
#include <meta/name.hpp>
#endif

namespace gal::prometheus::clp::regex
{
	#if not defined(CLP_IDENTIFIER)
	#define CLP_IDENTIFIER "[[:alnum:]][-_[:alnum:]\\.]*"
	#endif

	#if not defined(CLP_LIST_SEPARATOR)
	#define CLP_LIST_SEPARATOR ","
	#endif

	#define CLP_LIST_SEPARATOR_IGNORE_WS CLP_LIST_SEPARATOR "\\s*"

	using char_type = char;

	template<typename Range>
	concept string_type = std::ranges::contiguous_range<Range> and std::is_same_v<std::ranges::range_value_t<Range>, char_type>;

	using regex_type = std::basic_regex<char_type>;
	template<string_type Range>
	using regex_match_result_type = std::match_results<typename Range::const_iterator>;
	template<string_type Range>
	using regex_sub_match_type = typename regex_match_result_type<Range>::value_type;
	template<string_type Range>
	using regex_token_iterator = std::regex_token_iterator<typename Range::const_iterator>;

	template<string_type Range>
	[[nodiscard]] constexpr auto make(
		const Range& range,
		const regex_type::flag_type flag = regex_type::ECMAScript | regex_type::optimize
	) -> regex_type
	{
		return regex_type{std::ranges::data(range), std::ranges::size(range), flag};
	}

	template<string_type Range>
	[[nodiscard]] constexpr auto match(
		const Range& range,
		regex_match_result_type<Range>& result,
		const regex_type& regex
	) -> bool
	{
		return std::regex_match(std::ranges::begin(range), std::ranges::end(range), result, regex);
	}

	// ReSharper disable StringLiteralTypo
	// ReSharper disable CommentTypo
	// ReSharper disable CppTemplateArgumentsCanBeDeduced
	// ReSharper disable GrammarMistakeInComment

	constexpr std::basic_string_view<char_type> pattern_integer
	{
			// // result[0] -> [-/+] 0b1010101 / 0x123456789abcdef / 01234567 / 123456789
			// // result[1] -> "-" / "+" / ""
			// "([-+]?)"
			// "(?:"
			// // B
			// // result[2] -> "0b" / ""
			// // result[3] -> "1010101" / ""
			// "(0b)([01]+)"
			// "|" // or
			// // H
			// // result[4] -> "0x" / ""
			// // result[5] -> "123456789abcdef" / ""
			// "(0x)([0-9a-fA-F]+)"
			// "|"
			// // O
			// // result[6] -> "0" / ""
			// // result[7] -> "1234567" / ""
			// "(0)([0-7]+)"
			// "|" // or
			// // D
			// // result[8] -> "123456789" / ""
			// "([1-9][0-9]*"
			// "|" // or
			// // D
			// // result[8] -> "0" / ""
			// "0)"
			// ")"

			// result[0] -> [-/+] 0b1010101 / 0x123456789abcdef / 01234567 / 123456789
			// result[1] -> "-" / "+" / ""
			"([-+]?)"
			// result[2] -> "0b1010101" / "0x123456789abcdef" / "01234567" / "123456789"
			"("
			// B
			"0b[01]+"
			"|" // or
			// H
			"0x[0-9a-fA-F]+"
			"|" // or
			// O
			"0[0-7]*"
			"|" // or
			// D
			"[1-9][0-9]*"
			")"
	};

	constexpr std::basic_string_view<char_type> pattern_option
	{
			// --option-name / --option_name / --option.name 
			"--(" CLP_IDENTIFIER ")"
			// [= args]
			"(?:=(.*))?"
			// result[0] -> --option-name / --option_name / --option.name [= args]
			// result[1] -> option-name / option_name / option.name
			// result[2] -> args
			// result[3] -> ""
			"|" // or
			// -option-name / -option_name / -option.name
			"^-(?!-)(" CLP_IDENTIFIER ")$"
			// result[0] -> -option-name / -option_name / -option.name
			// result[1] -> ""
			// result[2] -> ""
			// result[3] -> option-name / option_name / option.name
	};

	// arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7
	// result[0] -> [arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7]
	constexpr std::basic_string_view<char_type> pattern_list
	{
			// arg1
			CLP_IDENTIFIER
			// [,arg2, arg3,  arg4,   arg5,    arg6,     arg7]
			"(?:" CLP_LIST_SEPARATOR_IGNORE_WS CLP_IDENTIFIER ")*"
	};
	// arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7
	// [arg1, arg2, arg3, arg4, arg5, arg6, arg7]
	constexpr std::basic_string_view<char_type> pattern_list_separator
	{
			CLP_LIST_SEPARATOR_IGNORE_WS
	};

	// ReSharper restore GrammarMistakeInComment
	// ReSharper restore CppTemplateArgumentsCanBeDeduced
	// ReSharper restore CommentTypo
	// ReSharper restore StringLiteralTypo

	using descriptor_boolean = bool;

	struct descriptor_integer
	{
		enum class Base : std::uint8_t
		{
			BINARY = 2,
			OCTAL = 8,
			DECIMAL = 10,
			HEXADECIMAL = 16,
		};

		bool is_negative;
		Base base;
		// Note that we don't copy the string, and always assume that parsing is done before the string is invalidated!
		std::basic_string_view<char_type> value;
	};

	struct descriptor_option
	{
		// Note that we don't copy the string, and always assume that parsing is done before the string is invalidated!
		std::basic_string_view<char_type> name;
		std::basic_string_view<char_type> value;
	};

	template<typename Range>
	[[nodiscard]] constexpr auto sub_match_to_string_view(
		const regex_sub_match_type<Range>& sub_match
	) noexcept -> std::basic_string_view<char_type>
	{
		return std::basic_string_view<char_type>{sub_match.first, sub_match.second};
	}

	template<string_type Range>
	using descriptor_list =
	std::remove_cvref_t<
		decltype( //
			std::ranges::subrange{std::declval<regex_token_iterator<Range>>(), std::declval<regex_token_iterator<Range>>()} | //
			std::views::transform(sub_match_to_string_view<Range>) | //
			std::views::common
		)
	>;

	template<bool True, string_type Range>
	[[nodiscard]] auto parse_boolean(const Range& range) -> descriptor_boolean
	{
		if constexpr (True)
		{
			constexpr static std::basic_string_view<char_type> candidates[]
			{
					"Y", "y",
					"YES", "Yes", "yes",
					"ON", "On", "on",
					"TRUE", "True", "true",
					"1"
			};

			return std::ranges::contains(candidates, range);
		}
		else
		{
			constexpr static std::basic_string_view<char_type> candidates[]
			{
					"N", "n",
					"NO", "No", "no",
					"OFF", "Off", "off",
					"FALSE", "False", "false",
					"0"
			};

			if (std::ranges::empty(range))
			{
				return true;
			}
			return std::ranges::contains(candidates, range);
		}
	}

	template<string_type Range>
	[[nodiscard]] auto parse_integer(const Range& range) ->
		#if CLP_USE_EXPECTED
		std::expected<descriptor_integer, std::string>
		#else
		std::optional<descriptor_integer>
		#endif
	{
		const static auto regex = regex::make(pattern_integer);

		regex_match_result_type<Range> result;
		const auto match_result = regex::match(range, result, regex);

		if (not match_result or result.size() != 3)
		{
			#if CLP_USE_EXPECTED
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<descriptor_integer>())};
			#else
			return std::nullopt;
			#endif
		}

		const auto is_negative = result[1] == '-';

		const auto& sub = result[2];
		const auto sub_string = regex::sub_match_to_string_view<Range>(sub);

		if (sub_string.starts_with("0b"))
		{
			return descriptor_integer
			{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::BINARY,
					.value = sub_string.substr(2)
			};
		}

		if (sub_string.starts_with("0x"))
		{
			return descriptor_integer
			{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::HEXADECIMAL,
					.value = sub_string.substr(2)
			};
		}

		if (sub_string.starts_with('0') and sub_string.size() > 1)
		{
			return descriptor_integer
			{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::OCTAL,
					.value = sub_string.substr(1)
			};
		}

		return descriptor_integer
		{
				.is_negative = is_negative,
				.base = descriptor_integer::Base::DECIMAL,
				.value = sub_string
		};
	}

	template<string_type Range>
	[[nodiscard]] auto parse_option(const Range& range) ->
		#if CLP_USE_EXPECTED
		std::expected<descriptor_option, std::string>
		#else
		std::optional<descriptor_option>
		#endif
	{
		const static auto regex = regex::make(pattern_option);

		regex_match_result_type<Range> result;
		const auto match_result = regex::match(range, result, regex);

		if (not match_result or (not result[1].matched and not result[3].matched))
		{
			#if CLP_USE_EXPECTED
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<descriptor_option>())};
			#else
			return std::nullopt;
			#endif
		}

		if (result[1].matched)
		{
			return descriptor_option
			{
					.name = regex::sub_match_to_string_view<Range>(result[1]),
					.value = regex::sub_match_to_string_view<Range>(result[2])
			};
		}

		return descriptor_option
		{
				.name = regex::sub_match_to_string_view<Range>(result[3]),
				.value = ""
		};
	}

	template<string_type Range>
	[[nodiscard]] auto parse_list(const Range& range) ->
		#if CLP_USE_EXPECTED
		std::expected<descriptor_list<Range>, std::string>
		#else
		std::optional<descriptor_list<Range>>
		#endif
	{
		const static auto regex = regex::make(pattern_list);
		const static auto regex_separator = regex::make(pattern_list_separator);

		regex_match_result_type<Range> result;
		const auto match_result = regex::match(range, result, regex);

		if (not match_result or result.size() != 1)
		{
			#if CLP_USE_EXPECTED
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<descriptor_list<Range>>())};
			#else
			return std::nullopt;
			#endif
		}

		regex_token_iterator<Range> token_iterator{result[0].first, result[0].second, regex_separator, -1};

		return std::ranges::subrange{token_iterator, regex_token_iterator<Range>{}} |
		       std::views::transform(sub_match_to_string_view<Range>) |
		       std::views::common;
	}

	struct parser
	{
		// boolean
		template<std::same_as<bool>, string_type Range>
		[[nodiscard]] constexpr static auto parse(const Range& range) ->
			#if CLP_USE_EXPECTED
			std::expected<bool, std::string>
			#else
			std::optional<bool>
			#endif
		{
			if (regex::parse_boolean<true>(range)) { return true; }

			if (regex::parse_boolean<false>(range)) { return false; }

			#if CLP_USE_EXPECTED
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<bool>())};
			#else
			return std::nullopt;
			#endif
		}

		// integral
		// not bool, avoid ambiguous
		template<std::integral T, string_type Range>
			requires(not std::is_same_v<T, bool>)
		[[nodiscard]] constexpr static auto parse(const Range& range) ->
			#if CLP_USE_EXPECTED
			std::expected<T, std::string>
			#else
			std::optional<T>
			#endif
		{
			#if CLP_USE_EXPECTED
			return regex::parse_integer(range)
					.and_then(
						[&range](const descriptor_integer& descriptor) -> std::expected<T, std::string>
						{
							const auto& [is_negative, base, value] = descriptor;
							const auto base_num = static_cast<int>(base);

							using type = std::make_unsigned_t<T>;

							type result;
							if (const auto [last, error] = std::from_chars(value.data(), value.data() + value.size(), result, base_num);
								error != std::errc{} or last != value.data() + value.size())
							{
								return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<type>())};
							}

							if (is_negative)
							{
								if constexpr (std::numeric_limits<T>::is_signed)
								{
									return static_cast<T>(-static_cast<T>(result - 1) - 1);
								}
								else
								{
									return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<type>())};
								}
							}

							return result;
						}
					);
			#else
			if (const auto descriptor = regex::parse_integer(range);
				descriptor.has_value())
			{
				const auto& [is_negative, base, value] = *descriptor;
				const auto base_num = static_cast<int>(base);

				using type = std::make_unsigned_t<T>;

				type result;
				if (const auto [last, error] = std::from_chars(value.data(), value.data() + value.size(), result, base_num);
					error != std::errc{} or last != value.data() + value.size())
				{
					return std::nullopt;
				}

				if (is_negative)
				{
					if constexpr (not std::numeric_limits<T>::is_signed)
					{
						return std::nullopt;
					}
					else
					{
						return static_cast<T>(-static_cast<T>(result - 1) - 1);
					}
				}

				return result;
			}

			return std::nullopt;
			#endif
		}

		/**
		 * @note MSVC is more permissive in how it handles two-phase lookup and constraint satisfaction,
		 * it effectively defers or relaxes some aspects of template instantiation until it sees the exact call.
		 * In contrast, Clang (and GCC) more strictly follows the standard rules for two-phase lookup and concepts.
		 * Consequently, when Clang sees the requires-expression involving something like(parser::parse):
		 * @code
		 * parser::parse<std::ranges::range_value_t<OutRange>>(
		 *		std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(
		 *			regex::parse_list(range).value()
		 *		)>>>()
		 * );
		 * @endcode
		 * it attempts to instantiate and check this call immediately during constraint checks.
		 * If no matching function was found (see no valid definition for that template function yet), Clang flags it as an error.
		 * MSVC, on the other hand, either does not perform this check as strictly or delays it,
		 * thus allowing the code to compile successfully in scenarios where Clang (and GCC) would reject it.
		 */

		// string / construct from string
		// not list, avoid ambiguous
		template<typename T, string_type Range>
			requires std::is_constructible_v<T, Range>
		[[nodiscard]] constexpr static auto parse(const Range& range) noexcept(std::is_nothrow_constructible_v<T, Range>) -> T
		{
			return T{range};
		}

		// list
		// not string, avoid ambiguous
		template<std::ranges::range OutRange, string_type Range>
			requires(not std::is_constructible_v<OutRange, Range>)
		constexpr static auto parse(const Range& range, OutRange& out) -> void //
			requires requires
			{
				parser::parse<std::ranges::range_value_t<OutRange>>(
					std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype( // descriptor_list -> descriptor_list::const_reference
						regex::parse_list(range).value() // range -> descriptor_list
					)>>>()
				);
			}
		{
			const auto list = regex::parse_list(range);
			if (not list.has_value())
			{
				return;
			}

			if constexpr (const auto& view = list.value();
				std::is_same_v<
					std::remove_cvref_t<decltype(
						parser::parse<std::ranges::range_value_t<OutRange>>(
							std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(view)>>>()
						)
					)>, //
					std::ranges::range_value_t<OutRange>
				> //
			)
			{
				std::ranges::for_each(
					view,
					[&out](const auto& string) -> void //
					{
						if constexpr (requires { out.emplace_back(parser::parse<std::ranges::range_value_t<OutRange>>(string)); }) //
						{
							out.emplace_back(parser::parse<std::ranges::range_value_t<OutRange>>(string));
						}
						else if constexpr (requires { out.push_back(parser::parse<std::ranges::range_value_t<OutRange>>(string)); }) //
						{
							out.push_back(parser::parse<std::ranges::range_value_t<OutRange>>(string));
						}
						else if constexpr (requires { out.emplace(parser::parse<std::ranges::range_value_t<OutRange>>(string)); }) //
						{
							out.emplace(parser::parse<std::ranges::range_value_t<OutRange>>(string));
						}
						else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
					});
			}
			else
			{
				OutRange tmp_out{};
				if constexpr (requires { tmp_out.reserve(std::ranges::distance(view)); }) { tmp_out.reserve(std::ranges::distance(view)); }

				for (auto it = std::ranges::begin(view); it != std::ranges::end(view); std::ranges::advance(it, 1))
				{
					auto value = parser::parse<std::ranges::range_value_t<OutRange>>(*it);
					if (not value.has_value()) { return; }

					if constexpr (requires { tmp_out.emplace_back(*std::move(value)); }) //
					{
						out.emplace_back(*std::move(value));
					}
					else if constexpr (requires { out.push_back(*std::move(value)); }) //
					{
						out.push_back(*std::move(value));
					}
					else if constexpr (requires { out.emplace(*std::move(value)); }) //
					{
						out.emplace(*std::move(value));
					}
					else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
				}

				if constexpr (requires { out.reserve(tmp_out.size()); }) { out.reserve(tmp_out.size() + out.size()); }
				std::ranges::move(tmp_out, std::back_inserter(out));
			}
		}

		// list
		// not string, avoid ambiguous
		template<std::ranges::range OutRange, string_type Range>
			requires(not std::is_constructible_v<OutRange, Range>) and
			        requires
			        {
				        parser::parse<OutRange, Range>(std::declval<const Range&>(), std::declval<OutRange&>());
			        }
		[[nodiscard]] constexpr static auto parse(const Range& range) -> OutRange
		{
			OutRange result{};

			parser::parse(range, result);

			return result;
		}
	};
}
