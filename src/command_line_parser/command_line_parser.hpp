// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <source_location>
#include <stacktrace>
#include <regex>
#include <ranges>
#include <print>
#include <unordered_map>
#include <iterator>

#if not defined(CLP_USE_EXPECTED)
#if defined(GAL_PROMETHEUS_COMPILER_DEBUG)
#define CLP_USE_EXPECTED 1
#else
#define CLP_USE_EXPECTED 0
#endif
#endif

#if CLP_USE_EXPECTED
#include <expected>
#else
#include <optional>
#endif

#include <prometheus/macro.hpp>

#include <platform/exception.hpp>
#include <platform/environment.hpp>
#include <string/string_pool.hpp>

namespace gal::prometheus::clp
{
	class CommandLineOptionNameFormatError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionNameFormatError>(
				std::format("Cannot parse `{}` as option name", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionAlreadyExistsError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionAlreadyExistsError>(
				std::format("Option `{}` already exists!", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionUnrecognizedError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionUnrecognizedError>(
				std::format("Unrecognized option:\n {}", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionRequiredNotPresentError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionRequiredNotPresentError>(
				std::format("Required option `{}` not present", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	class CommandLineOptionRequiredNotSetError final : public platform::Exception<void>
	{
	public:
		using Exception::Exception;

		[[noreturn]] static auto panic(
			const std::string_view option,
			const std::source_location& location = std::source_location::current(),
			std::stacktrace stacktrace = std::stacktrace::current()
		) noexcept(false) -> void //
		{
			platform::panic<CommandLineOptionRequiredNotSetError>(
				std::format("Required option `{}` not set and no default value present", option),
				location,
				std::move(stacktrace)
			);
		}
	};

	namespace clp_detail
	{
		#if not defined(CLP_IDENTIFIER)
		#define CLP_IDENTIFIER "[[:alnum:]][-_[:alnum:]\\.]*"
		#endif

		#if not defined(CLP_LIST_SEPARATOR)
		#define CLP_LIST_SEPARATOR ","
		#endif

		#define CLP_LIST_SEPARATOR_IGNORE_WS CLP_LIST_SEPARATOR "\\s*"

		using regex_char_type = char;

		template<typename Range>
		concept regex_string_type = std::ranges::contiguous_range<Range> and std::is_same_v<std::ranges::range_value_t<Range>, regex_char_type>;

		using regex_type = std::basic_regex<regex_char_type>;
		template<regex_string_type Range>
		using regex_match_result_type = std::match_results<typename Range::const_iterator>;
		template<regex_string_type Range>
		using regex_sub_match_type = typename regex_match_result_type<Range>::value_type;
		template<regex_string_type Range>
		using regex_token_iterator = std::regex_token_iterator<typename Range::const_iterator>;

		template<regex_string_type Range>
		[[nodiscard]] constexpr auto make_regex(
			const Range& range,
			const regex_type::flag_type flag = regex_type::ECMAScript | regex_type::optimize
		) -> regex_type
		{
			return regex_type{std::ranges::data(range), std::ranges::size(range), flag};
		}

		template<regex_string_type Range>
		[[nodiscard]] constexpr auto regex_match(const Range& range, regex_match_result_type<Range>& result, const regex_type& regex) -> bool
		{
			return std::regex_match(std::ranges::begin(range), std::ranges::end(range), result, regex);
		}

		// ReSharper disable StringLiteralTypo
		// ReSharper disable CommentTypo
		// ReSharper disable CppTemplateArgumentsCanBeDeduced
		// ReSharper disable GrammarMistakeInComment

		constexpr std::basic_string_view<regex_char_type> pattern_integer
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

		constexpr std::basic_string_view<regex_char_type> pattern_option
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
		constexpr std::basic_string_view<regex_char_type> pattern_list
		{
				// arg1
				CLP_IDENTIFIER
				// [,arg2, arg3,  arg4,   arg5,    arg6,     arg7]
				"(?:" CLP_LIST_SEPARATOR_IGNORE_WS CLP_IDENTIFIER ")*"
		};
		// arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7
		// [arg1, arg2, arg3, arg4, arg5, arg6, arg7]
		constexpr std::basic_string_view<regex_char_type> pattern_list_separator
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
			std::basic_string_view<regex_char_type> value;
		};

		struct descriptor_option
		{
			// Note that we don't copy the string, and always assume that parsing is done before the string is invalidated!
			std::basic_string_view<regex_char_type> name;
			std::basic_string_view<regex_char_type> value;
		};

		template<typename Range>
		[[nodiscard]] constexpr auto sub_match_to_string_view(const regex_sub_match_type<Range>& sub_match)
			noexcept -> std::basic_string_view<regex_char_type> //
		{
			return std::basic_string_view<regex_char_type>{sub_match.first, sub_match.second};
		}

		template<regex_string_type Range>
		using descriptor_list =
		std::remove_cvref_t<
			decltype( //
				std::ranges::subrange{std::declval<regex_token_iterator<Range>>(), std::declval<regex_token_iterator<Range>>()} | //
				std::views::transform(sub_match_to_string_view<Range>) | //
				std::views::common
			)
		>;

		template<bool True, regex_string_type Range>
		[[nodiscard]] auto parse_boolean(const Range& range) -> descriptor_boolean
		{
			if constexpr (True)
			{
				constexpr static std::basic_string_view<regex_char_type> candidates[]
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
				constexpr static std::basic_string_view<regex_char_type> candidates[]
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

		template<regex_string_type Range>
		[[nodiscard]] auto parse_integer(const Range& range) ->
			#if CLP_USE_EXPECTED
			std::expected<descriptor_integer, std::string>
			#else
			std::optional<descriptor_integer>
			#endif
		{
			const static auto regex = make_regex(pattern_integer);

			regex_match_result_type<Range> result;
			const auto match_result = clp_detail::regex_match(range, result, regex);

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
			const auto sub_string = clp_detail::sub_match_to_string_view<Range>(sub);

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

		template<regex_string_type Range>
		[[nodiscard]] auto parse_option(const Range& range) ->
			#if CLP_USE_EXPECTED
			std::expected<descriptor_option, std::string>
			#else
			std::optional<descriptor_option>
			#endif
		{
			const static auto regex = make_regex(pattern_option);

			regex_match_result_type<Range> result;
			const auto match_result = clp_detail::regex_match(range, result, regex);

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
						.name = clp_detail::sub_match_to_string_view<Range>(result[1]),
						.value = clp_detail::sub_match_to_string_view<Range>(result[2])
				};
			}

			return descriptor_option
			{
					.name = clp_detail::sub_match_to_string_view<Range>(result[3]),
					.value = ""
			};
		}

		template<regex_string_type Range>
		[[nodiscard]] auto parse_list(const Range& range) ->
			#if CLP_USE_EXPECTED
			std::expected<descriptor_list<Range>, std::string>
			#else
			std::optional<descriptor_list<Range>>
			#endif
		{
			const static auto regex = make_regex(pattern_list);
			const static auto regex_separator = make_regex(pattern_list_separator);

			regex_match_result_type<Range> result;
			const auto match_result = clp_detail::regex_match(range, result, regex);

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
			template<typename T, regex_string_type Range>
				requires std::is_same_v<T, bool>
			[[nodiscard]] constexpr static auto parse(const Range& range) ->
				#if CLP_USE_EXPECTED
				std::expected<bool, std::string>
				#else
				std::optional<bool>
				#endif
			{
				if (clp_detail::parse_boolean<true>(range)) { return true; }

				if (clp_detail::parse_boolean<false>(range)) { return false; }

				#if CLP_USE_EXPECTED
				return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<bool>())};
				#else
				return std::nullopt;
				#endif
			}

			// integral
			// not bool, avoid ambiguous
			template<std::integral T, regex_string_type Range>
				requires(not std::is_same_v<T, bool>)
			[[nodiscard]] constexpr static auto parse(const Range& range) ->
				#if CLP_USE_EXPECTED
				std::expected<T, std::string>
				#else
				std::optional<T>
				#endif
			{
				#if CLP_USE_EXPECTED
				return clp_detail::parse_integer(range)
						.and_then(
							[&range](const descriptor_integer& descriptor) -> std::expected<T, std::string>
							{
								const auto& [is_negative, base, value] = descriptor;
								const auto base_num = static_cast<int>(base);

								using type = std::make_unsigned_t<T>;
								type result;
								if (
									const auto [last, error] = std::from_chars(value.data(), value.data() + value.size(), result, base_num);
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
				if (const auto descriptor = clp_detail::parse_integer(range);
					descriptor.has_value())
				{
					const auto& [is_negative, base, value] = *descriptor;
					const auto base_num = static_cast<int>(base);

					using type = std::make_unsigned_t<T>;
					type result;
					if (
						const auto [last, error] = std::from_chars(value.data(), value.data() + value.size(), result, base_num);
						error != std::errc{} or last != value.data() + value.size()) { return std::nullopt; }

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

			// list
			// not string, avoid ambiguous
			template<std::ranges::range OutRange, regex_string_type Range>
				requires(not std::is_constructible_v<OutRange, Range>)
			constexpr static auto parse(const Range& range, OutRange& out) -> void //
				requires requires
				{
					parser::parse<std::ranges::range_value_t<OutRange>>(
						std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype( // descriptor_list -> descriptor_list::const_reference
							clp_detail::parse_list(range).value() // range -> descriptor_list
						)>>>()
					);
				}
			{
				const auto list = clp_detail::parse_list(range);
				if (not list.has_value())
				{
					return;
				}

				if constexpr (const auto view = *list;
					std::is_same_v<
						std::remove_cvref_t<decltype(parser::parse<std::ranges::range_value_t<OutRange>>(
							std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(view)>>>()))>, //
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
			template<std::ranges::range OutRange, regex_string_type Range>
				requires(not std::is_constructible_v<OutRange, Range>) and
				        requires
				        {
					        parser::parse(std::declval<const Range&>(), std::declval<OutRange&>());
				        }
			[[nodiscard]] constexpr static auto parse(const Range& range) -> OutRange
			{
				OutRange result{};

				parser::parse(range, result);

				return result;
			}

			// string / construct from string
			// not list, avoid ambiguous
			template<typename T, regex_string_type Range>
				requires std::is_constructible_v<T, Range>
			[[nodiscard]] constexpr static auto parse(const Range& range) noexcept(std::is_nothrow_constructible_v<T, Range>) -> T
			{
				return T{range};
			}
		};
	}

	template<clp_detail::regex_string_type StringType, clp_detail::regex_string_type StringViewType>
	class CommandLineOption;
	template<clp_detail::regex_string_type StringType = std::basic_string<clp_detail::regex_char_type>>
	class CommandLineOptionParser;

	template<clp_detail::regex_string_type StringType, clp_detail::regex_string_type StringViewType>
	class CommandLineOption
	{
	public:
		using string_type = StringType;
		using string_view_type = StringViewType;

		using parser_type = CommandLineOptionParser<string_type>;
		friend parser_type;

	private:
		struct value_type;

		struct implicit_value_type;

		struct default_value_type
		{
			string_view_type value;

			[[nodiscard]] constexpr auto operator+(const implicit_value_type& v) const noexcept -> value_type
			{
				return {*this, v};
			}

			// ReSharper disable once CppNonExplicitConversionOperator
			[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept
			{
				return value;
			}
		};

		struct implicit_value_type
		{
			string_view_type value;

			[[nodiscard]] constexpr auto operator+(const default_value_type& v) const noexcept -> value_type
			{
				return {v, *this};
			}

			// ReSharper disable once CppNonExplicitConversionOperator
			[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept
			{
				return value;
			}
		};

		struct value_type
		{
			default_value_type dv;
			implicit_value_type iv;
		};

	public:
		[[nodiscard]] constexpr static auto default_value(const string_view_type value) noexcept -> default_value_type
		{
			return {value};
		}

		[[nodiscard]] constexpr static auto implicit_value(const string_view_type value) noexcept -> implicit_value_type
		{
			return {value};
		}

	private:
		// This string has no meaning but can indicate that the current value is set.
		constexpr static string_view_type secret_value{"~!@#$%^&*()_+"};

		string_view_type option_short_format_;
		string_view_type option_long_format_;

		value_type value_;
		string_view_type current_value_;

		constexpr auto set_value_default() noexcept -> void
		{
			current_value_ = value_.dv;
		}

		constexpr auto set_value_implicit() noexcept -> void
		{
			current_value_ = value_.iv;
		}

		constexpr auto set_value(const string_view_type value = secret_value) noexcept -> void
		{
			current_value_ = value;
		}

		template<typename AllocateFunction>
			requires std::is_same_v<std::invoke_result_t<AllocateFunction, string_view_type>, string_view_type>
		constexpr auto respawn(AllocateFunction allocator) -> void
		{
			option_short_format_ = allocator(option_short_format_);
			option_long_format_ = allocator(option_long_format_);

			if (not value_.dv.value.empty())
			{
				value_.dv = {allocator(value_.dv.value)};
			}
			if (not value_.iv.value.empty())
			{
				value_.iv = {allocator(value_.iv.value)};
			}

			set_value_implicit();
		}

	public:
		constexpr CommandLineOption(
			const string_view_type option_short_format,
			const string_view_type option_long_format,
			const value_type value
		) noexcept
			: option_short_format_{option_short_format},
			  option_long_format_{option_long_format},
			  value_{value},
			  // `Defaults` use implicit_value,
			  // if the option is given `explicitly` the `default_value` is used,
			  // if the option is given `explicitly` and a value is specified the specified value is used.
			  current_value_{value_.iv} {}

		[[nodiscard]] constexpr auto set() const noexcept -> bool
		{
			return not current_value_.empty();
		}

		[[nodiscard]] constexpr auto has_default() const noexcept -> bool
		{
			return not value_.dv.empty();
		}

		[[nodiscard]] constexpr auto default_value() const noexcept -> string_view_type
		{
			return value_.dv;
		}

		[[nodiscard]] constexpr auto is_default() const noexcept -> bool
		{
			return current_value_ == default_value();
		}

		[[nodiscard]] constexpr auto has_implicit() const noexcept -> bool
		{
			return not value_.iv.empty();
		}

		[[nodiscard]] constexpr auto implicit_value() const noexcept -> string_view_type
		{
			return value_.iv;
		}

		[[nodiscard]] constexpr auto is_implicit() const noexcept -> bool
		{
			return current_value_ == implicit_value();
		}

		template<typename T>
			requires requires
			{
				clp_detail::parser::parse<T>(std::declval<string_view_type>());
			}
		[[nodiscard]] constexpr auto as() const -> std::optional<T>
		{
			if (not set())
			{
				return std::nullopt;
			}

			if constexpr (
				std::is_same_v<
					std::remove_cvref_t<decltype(clp_detail::parser::parse<T>(std::declval<string_view_type>()))>,
					T
				>
			)
			{
				return clp_detail::parser::parse<T>(current_value_);
			}
			else
			{
				auto value = clp_detail::parser::parse<T>(current_value_);
				#if CLP_USE_EXPECTED
				if (value.has_value())
				{
					return *std::move(value);
				}

				std::println(stderr, "Cannot parse `{}` as `{}`.", current_value_, meta::name_of<T>());
				return std::nullopt;
				#else
				return value;
				#endif
			}
		}
	};

	template<clp_detail::regex_string_type StringType>
	class CommandLineOptionParser
	{
	public:
		using string_type = StringType;
		using string_pool_type = string::StringPool<typename string_type::value_type, false>;
		using string_view_type = typename string_pool_type::view_type;

		using option_type = CommandLineOption<string_type, string_view_type>;
		using option_list_type = std::unordered_map<string_view_type, std::shared_ptr<option_type>>;
		using option_list_size_type = typename option_list_type::size_type;

		[[nodiscard]] constexpr static auto default_value(const string_view_type value) noexcept -> typename option_type::default_value_type
		{
			return option_type::default_value(value);
		}

		[[nodiscard]] constexpr static auto implicit_value(const string_view_type value) noexcept -> typename option_type::implicit_value_type
		{
			return option_type::implicit_value(value);
		}

	private:
		string_pool_type string_pool_;

		option_list_type option_list_;

		bool allow_unrecognized_;

		constexpr auto do_add_option(const std::shared_ptr<option_type>& option) -> void
		{
			option->respawn([this](const string_view_type value) -> string_view_type
			{
				return string_pool_.add(value);
			});

			auto add = [this](const string_view_type name, std::shared_ptr<option_type> o) -> void
			{
				const auto [_, inserted] = option_list_.emplace(name, std::move(o));
				if (not inserted)
				{
					CommandLineOptionAlreadyExistsError::panic(name);
				}
			};

			if (not option->option_short_format_.empty())
			{
				add(option->option_short_format_, option);
			}
			add(option->option_long_format_, option);
		}

		constexpr auto do_add_alias(const string_view_type alias_name, const string_view_type target_option_name) -> void
		{
			const auto target_option_it = option_list_.find(target_option_name);
			if (target_option_it == option_list_.end())
			{
				CommandLineOptionRequiredNotPresentError::panic(target_option_name);
			}

			if (const auto it = option_list_.find(alias_name);
				it != option_list_.end())
			{
				CommandLineOptionAlreadyExistsError::panic(alias_name);
			}

			option_list_.emplace(alias_name, target_option_it->second);
		}

	public:
		constexpr explicit CommandLineOptionParser(const bool allow_unrecognized = false) noexcept
			: allow_unrecognized_{allow_unrecognized} {}

		constexpr auto add_option(const string_view_type option, const typename option_type::value_type value) -> CommandLineOptionParser&
		{
			const auto option_names = clp_detail::parse_list(option);
			if (not option_names.has_value())
			{
				CommandLineOptionNameFormatError::panic(option);
			}

			const auto& option_view = *option_names;
			const auto option_size = std::ranges::distance(option_view);

			if (option_size != 1 and option_size != 2)
			{
				CommandLineOptionNameFormatError::panic(option);
			}

			const auto o1 = string_view_type{*std::ranges::begin(option_view)};
			const auto o2 = option_size == 2
				                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
				                : string_view_type{};

			const auto short_format = o1.size() < o2.size() ? o1 : o2;
			const auto long_format = o1.size() < o2.size() ? o2 : o1;

			auto o = std::make_shared<option_type>(short_format, long_format, value);
			this->do_add_option(o);

			return *this;
		}

		constexpr auto add_option(const string_view_type option, const typename option_type::implicit_value_type value) -> CommandLineOptionParser&
		{
			return this->add_option(option, value + typename option_type::default_value_type{});
		}

		constexpr auto add_option(const string_view_type option, const typename option_type::default_value_type value) -> CommandLineOptionParser&
		{
			return this->add_option(option, value + typename option_type::implicit_value_type{});
		}

		constexpr auto add_option(const string_view_type option) -> CommandLineOptionParser&
		{
			return this->add_option(option, typename option_type::value_type{});
		}

		constexpr auto add_alias(const string_view_type alias_name, const string_view_type target_option_name) -> CommandLineOptionParser&
		{
			const auto option_names = clp_detail::parse_list(alias_name);
			if (not option_names.has_value())
			{
				CommandLineOptionNameFormatError::panic(alias_name);
			}

			const auto& option_view = *option_names;
			const auto option_size = std::ranges::distance(option_view);

			if (option_size != 1 and option_size != 2)
			{
				CommandLineOptionNameFormatError::panic(alias_name);
			}

			const auto o1 = string_view_type{*std::ranges::begin(option_view)};
			const auto o2 = option_size == 2
				                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
				                : string_view_type{};

			this->do_add_alias(o1, target_option_name);
			if (not o2.empty())
			{
				this->do_add_alias(o2, target_option_name);
			}

			return *this;
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		constexpr auto parse(const Begin begin, const End end) -> void
		{
			std::vector<string_view_type> unmatched{};

			std::ranges::for_each(
				begin,
				end,
				[this, &unmatched](const auto& string) -> void
				{
					const auto option = clp_detail::parse_option(string);

					if (not option.has_value())
					{
						unmatched.emplace_back(string);
						return;
					}

					auto it = option_list_.find(option->name);
					if (it == option_list_.end())
					{
						unmatched.emplace_back(option->name);
						return;
					}

					// Since we allow `--option`, we are not sure here whether the string is `--option` or `-option`.
					if (option->value.empty())
					{
						#if __cpp_lib_ranges_starts_ends_with >= 202106L
						if (std::ranges::starts_with(string, string_view_type{"--"}))
						#else
						if (std::ranges::equal(string_view_type{"--"}, string))
						#endif
						{
							// --option
							it->second->set_value_default();
						}
						else
						{
							// -option
							it->second->set_value();
						}
					}
					else
					{
						const auto allocated_value = string_pool_.add(option->value);
						it->second->set_value(allocated_value);
					}
				}
			);

			if (not unmatched.empty())
			{
				string_type options{};

				std::ranges::for_each(
					unmatched,
					[&options](const string_view_type option)
					{
						options.push_back('\t');
						options.append(" - ");
						options.append(option);
						options.push_back('\n');
					}
				);

				std::println(stderr, "Unrecognized option:\n {}", options);

				if (not allow_unrecognized_)
				{
					CommandLineOptionUnrecognizedError::panic(options);
				}
			}
		}

		template<std::ranges::input_range Range>
		constexpr auto parse(const Range& range) -> void
		{
			this->parse(std::ranges::begin(range), std::ranges::end(range));
		}

		auto parse() -> void
		{
			const auto args = platform::command_args();
			this->parse(
				args |
				std::views::drop(1) | // skip argv[0]
				std::views::transform(
					[](const auto& p) noexcept -> std::string_view
					{
						return {p};
					}
				)
			);
		}

		[[nodiscard]] constexpr auto contains(const string_view_type arg_name) const -> bool
		{
			return option_list_.contains(arg_name);
		}

		[[nodiscard]] constexpr auto count(const string_view_type arg_name) const -> option_list_size_type
		{
			return option_list_.count(arg_name);
		}

		[[nodiscard]] constexpr auto operator[](const string_view_type arg_name) const -> const option_type&
		{
			const auto it = option_list_.find(arg_name);
			if (it == option_list_.end())
			{
				CommandLineOptionRequiredNotPresentError::panic(arg_name);
			}

			return *it->second;
		}
	};
}
