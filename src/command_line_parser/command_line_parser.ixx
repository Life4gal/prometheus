// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_COMPILER_MSVC
#include <cstdlib>
#endif

export module gal.prometheus:command_line_parser;

import std;

import :platform;
import :meta;
import :functional;
import :string;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <source_location>
#include <stacktrace>
#include <regex>
#include <ranges>
#include <expected>
#include <iostream>
#include <unordered_map>
#include <iterator>

#include <prometheus/macro.hpp>

#include <platform/platform.ixx>
#include <meta/meta.ixx>
#include <functional/functional.ixx>
#include <string/string.ixx>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: clp
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(clp)
#endif
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
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(clp)
{
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
	auto make_regex(const Range& range) -> regex_type { return regex_type{std::ranges::data(range), std::ranges::size(range)}; }

	template<regex_string_type Range>
	auto regex_match(const Range& range, regex_match_result_type<Range>& result, const regex_type& regex) -> bool
	{
		return std::regex_match(std::ranges::begin(range), std::ranges::end(range), result, regex);
	}

	// ReSharper disable StringLiteralTypo
	// ReSharper disable CppTemplateArgumentsCanBeDeduced
	#define LIST_SEPARATOR_CHAR ','
	#define LIST_SEPARATOR_CHARS ","
	constexpr regex_char_type list_separator_char{LIST_SEPARATOR_CHAR};
	constexpr std::basic_string_view<regex_char_type> list_separator_chars{LIST_SEPARATOR_CHARS};

	constexpr std::basic_string_view<regex_char_type> pattern_boolean_true{"(t|T)(rue)?|1"};
	constexpr std::basic_string_view<regex_char_type> pattern_boolean_false{"(f|F)(alse)?|0"};
	// result[1] -> sign[-/+/]
	// result[2] -> 0b1010101 / 01234567 / 123456789 / 0x123456789abcdef
	constexpr std::basic_string_view<regex_char_type> pattern_integer{"([+-]?)(0b[01]+|0x[0-9a-fA-F]+|0[0-7]*|[1-9][0-9]*)"};
	// --option-name / --option_name / --option.name [= args]
	// result[1] -> option-name / option_name / option.name
	// result[2] -> =args
	// result[3] -> args
	// -option-name / -option_name / -option.name
	// result[4] -> option-name / option_name / option.name
	constexpr std::basic_string_view<regex_char_type> pattern_option{"--([[:alnum:]][-_[:alnum:]\\.]+)(=(.*))?|-([[:alnum:]].*)"};
	// arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7
	// result[0] -> [arg1,arg2, arg3,  arg4,   arg5,    arg6,     arg7]
	// result[1] -> [arg1]
	// result[2] -> [,     arg7]
	constexpr std::basic_string_view<regex_char_type> pattern_list{
		"([[:alnum:]][-_[:alnum:]\\.]*)(" LIST_SEPARATOR_CHARS "\\s*[[:alnum:]][-_[:alnum:]]*)*"};
	constexpr std::basic_string_view<regex_char_type> pattern_list_separator{LIST_SEPARATOR_CHARS "\\s*"};
	// ReSharper restore CppTemplateArgumentsCanBeDeduced
	// ReSharper restore StringLiteralTypo

	#if defined(GAL_PROMETHEUS_COMPILER_DEBUG)
	#define GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED 1
	#endif

	using descriptor_boolean = bool;

	struct descriptor_integer
	{
		enum class Base
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
		const static auto regex_true = make_regex(pattern_boolean_true);
		const static auto regex_false = make_regex(pattern_boolean_false);

		regex_match_result_type<Range> result;
		if constexpr (True) { regex_match(range, result, regex_true); }
		else { regex_match(range, result, regex_false); }
		return not result.empty();
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_integer(const Range& range) ->
		#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
		std::expected<descriptor_integer, std::string>
		#else
		std::optional<descriptor_integer>
		#endif
	{
		const static auto regex = make_regex(pattern_integer);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 3)
		{
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<descriptor_integer>())};
			#else
			return std::nullopt;
			#endif
		}

		const auto is_negative = result[1] == '-';

		const auto& sub = result[2];
		const std::basic_string_view sub_string{sub.first, sub.second};
		if (sub_string.starts_with("0b"))
		{
			return descriptor_integer{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::BINARY,
					.value = sub_string.substr(2)};
		}
		if (sub_string.starts_with("0x"))
		{
			return descriptor_integer{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::HEXADECIMAL,
					.value = sub_string.substr(2)};
		}
		if (sub_string.starts_with('0'))
		{
			return descriptor_integer{
					.is_negative = is_negative,
					.base = descriptor_integer::Base::OCTAL,
					.value = sub_string.substr(1)};
		}
		return descriptor_integer{
				.is_negative = is_negative,
				.base = descriptor_integer::Base::DECIMAL,
				.value = sub_string};
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_option(const Range& range) ->
		#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
		std::expected<descriptor_option, std::string>
		#else
		std::optional<descriptor_option>
		#endif
	{
		const static auto regex = make_regex(pattern_option);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 5)
		{
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<descriptor_option>())};
			#else
			return std::nullopt;
			#endif
		}

		if (result.length(4) > 0)
		{
			return descriptor_option{
					.name = {result[4].first, result[4].second},
					.value = ""};
		}

		return descriptor_option{
				.name = {result[1].first, result[1].second},
				.value = {result[3].first, result[3].second}};
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_list(const Range& range) ->
		#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
		std::expected<descriptor_list<Range>, std::string>
		#else
		std::optional<descriptor_list<Range>>
		#endif
	{
		const static auto regex = make_regex(pattern_list);
		const static auto regex_separator = make_regex(pattern_list_separator);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 3)
		{
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
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

	namespace parser
	{
		// boolean
		template<typename T, regex_string_type Range>
			requires std::is_same_v<T, bool>
		[[nodiscard]] constexpr auto parse(const Range& range) ->
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			std::expected<bool, std::string>
			#else
			std::optional<bool>
			#endif
		{
			if (parse_boolean<true>(range)) { return true; }

			if (parse_boolean<false>(range)) { return false; }

			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<bool>())};
			#else
			return std::nullopt;
			#endif
		}

		// integral
		// not bool, avoid ambiguous
		template<std::integral T, regex_string_type Range>
			requires(not std::is_same_v<T, bool>)
		[[nodiscard]] constexpr auto parse(const Range& range) ->
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			std::expected<T, std::string>
			#else
			std::optional<T>
			#endif
		{
			#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
			return parse_integer(range)
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
								if constexpr (not std::numeric_limits<T>::is_signed)
								{
									return std::unexpected{std::format("Cannot parse `{}` as `{}`.", range, meta::name_of<type>())};
								}

								return static_cast<T>(-static_cast<T>(result - 1) - 1);
							}

							return result;
						});
			#else
			if (const auto descriptor = parse_integer(range);
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
					if constexpr (not std::numeric_limits<T>::is_signed) { return std::nullopt; }

					return static_cast<T>(-static_cast<T>(result - 1) - 1);
				}

				return result;
			}

			return std::nullopt;
			#endif
		}

		// list
		// not string, avoid ambiguous
		template<std::ranges::range OutRange, regex_string_type Range>
			requires(not std::is_constructible_v<OutRange, Range>) and
			        requires
			        {
				        parser::parse<std::ranges::range_value_t<OutRange>>(
					        std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(
						        parse_list(std::declval<const Range&>()).value()
					        )>>>()
				        );
			        }
		constexpr auto parse(const Range& range, OutRange& out) -> void
		{
			const auto list = parse_list(range);
			if (not list.has_value()) { return; }

			if constexpr (const auto view = *list;
				std::is_same_v<
					std::remove_cvref_t<decltype(parser::parse<std::ranges::range_value_t<OutRange>>(
						std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(view)>>>()))>, //
					std::ranges::range_value_t<OutRange>> //
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
		[[nodiscard]] constexpr auto parse(const Range& range) -> OutRange
		{
			OutRange result{};

			parser::parse(range, result);

			return result;
		}

		// string / construct from string
		// not list, avoid ambiguous
		template<typename T, regex_string_type Range>
			requires std::is_constructible_v<T, Range>
		[[nodiscard]] constexpr auto parse(const Range& range) noexcept(std::is_nothrow_constructible_v<T, Range>) -> T { return T{range}; }
	}
}

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: clp
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(clp)
#endif
{
	template<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringType, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringViewType>
	class CommandLineOption;
	template<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringType = std::basic_string<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_char_type>>
	class CommandLineOptionParser;

	template<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringType, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringViewType>
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

			[[nodiscard]] constexpr auto operator+(const implicit_value_type& v) const noexcept -> value_type { return {*this, v}; }

			[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept { return value; }
		};

		struct implicit_value_type
		{
			string_view_type value;

			[[nodiscard]] constexpr auto operator+(const default_value_type& v) const noexcept -> value_type { return {v, *this}; }

			[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept { return value; }
		};

		struct value_type
		{
			default_value_type dv;
			implicit_value_type iv;
		};

	public:
		[[nodiscard]] constexpr static auto default_value(const string_view_type value) noexcept -> default_value_type { return {value}; }

		[[nodiscard]] constexpr static auto implicit_value(const string_view_type value) noexcept -> implicit_value_type { return {value}; }

	private:
		// This string has no meaning but can indicate that the current value is set.
		constexpr static string_view_type secret_value{"~!@#$%^&*()_+"};

		string_view_type option_short_format_;
		string_view_type option_long_format_;

		value_type value_;
		string_view_type current_value_;

		constexpr auto set_value_default() noexcept -> void { current_value_ = value_.dv; }

		constexpr auto set_value_implicit() noexcept -> void { current_value_ = value_.iv; }

		constexpr auto set_value(const string_view_type value = secret_value) noexcept -> void { current_value_ = value; }

		template<typename AllocateFunction>
			requires std::is_same_v<std::invoke_result_t<AllocateFunction, string_view_type>, string_view_type>
		constexpr auto respawn(AllocateFunction allocator) -> void
		{
			option_short_format_ = allocator(option_short_format_);
			option_long_format_ = allocator(option_long_format_);

			if (not value_.dv.value.empty()) { value_.dv = {allocator(value_.dv.value)}; }
			if (not value_.iv.value.empty()) { value_.iv = {allocator(value_.iv.value)}; }

			set_value_implicit();
		}

	public:
		constexpr CommandLineOption(
			const string_view_type option_short_format,
			const string_view_type option_long_format,
			const value_type value) noexcept
			: option_short_format_{option_short_format},
			  option_long_format_{option_long_format},
			  value_{value},
			  // `Defaults` use implicit_value, if the option is given `explicitly` the `default_value` is used, if the option is given `explicitly` and a value is specified the specified value is used.
			  current_value_{value_.iv} {}

		[[nodiscard]] constexpr auto set() const noexcept -> bool { return not current_value_.empty(); }

		[[nodiscard]] constexpr auto has_default() const noexcept -> bool { return not value_.dv.empty(); }

		[[nodiscard]] constexpr auto default_value() const noexcept -> string_view_type { return value_.dv; }

		[[nodiscard]] constexpr auto is_default() const noexcept -> bool { return current_value_ == default_value(); }

		[[nodiscard]] constexpr auto has_implicit() const noexcept -> bool { return not value_.iv.empty(); }

		[[nodiscard]] constexpr auto implicit_value() const noexcept -> string_view_type { return value_.iv; }

		[[nodiscard]] constexpr auto is_implicit() const noexcept -> bool { return current_value_ == implicit_value(); }

		template<typename T>
			requires requires
			{
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parser::parse<T>(std::declval<string_view_type>());
			}
		[[nodiscard]] auto as() const -> std::optional<T>
		{
			if (not set()) { return std::nullopt; }

			if constexpr (
				std::is_same_v<
					std::remove_cvref_t<decltype(GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parser::parse<T>(std::declval<string_view_type>()))>,
					T>
			)
			{
				return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parser::parse(current_value_);
			}
			else
			{
				auto value = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parser::parse<T>(current_value_);
				#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
				if (value.has_value()) { return *std::move(value); }

				std::cerr << std::format("Cannot parse `{}` as `{}`.", current_value_, meta::name_of<T>());
				return std::nullopt;
				#else
				return value;
				#endif
			}
		}
	};

	template<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::regex_string_type StringType>
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

		auto add_option(std::shared_ptr<option_type> option) -> void
		{
			option->respawn([this](const string_view_type value) -> string_view_type { return string_pool_.append(value); });

			auto do_add_option = [this](const string_view_type name, std::shared_ptr<option_type> o) -> void
			{
				const auto [_, inserted] = option_list_.emplace(name, std::move(o));
				if (not inserted) { CommandLineOptionAlreadyExistsError::panic(name); }
			};

			if (not option->option_short_format_.empty()) { do_add_option(option->option_short_format_, option); }
			do_add_option(option->option_long_format_, option);
		}

		auto add_alias(const string_view_type alias_name, const string_view_type target_option_name) -> void
		{
			const auto target_option_it = option_list_.find(target_option_name);
			if (target_option_it == option_list_.end()) { CommandLineOptionRequiredNotPresentError::panic(target_option_name); }

			if (const auto it = option_list_.find(alias_name);
				it != option_list_.end()) { CommandLineOptionAlreadyExistsError::panic(alias_name); }

			option_list_.emplace(alias_name, target_option_it->second);
		}

	public:
		explicit CommandLineOptionParser(const bool allow_unrecognized = false) noexcept
			: allow_unrecognized_{allow_unrecognized} {}

		[[nodiscard]] auto options() noexcept -> auto
		{
			return functional::y_combinator{
					functional::overloaded{
							[this](auto& self, const string_view_type option, const typename option_type::value_type value) -> auto& {
								const auto option_names = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parse_list(option);
								if (not option_names.has_value()) { CommandLineOptionNameFormatError::panic(option); }

								const auto option_view = *option_names;
								const auto option_size = std::ranges::distance(option_view);

								if (option_size != 1 and option_size != 2) { CommandLineOptionNameFormatError::panic(option); }

								const auto o1 = string_view_type{*std::ranges::begin(option_view)};
								const auto o2 = option_size == 2
									                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
									                : string_view_type{};

								const auto short_format = o1.size() < o2.size() ? o1 : o2;
								const auto long_format = o1.size() < o2.size() ? o2 : o1;

								auto o = std::make_shared<option_type>(short_format, long_format, value);
								add_option(o);

								return self;
							},
							[](auto& self, const string_view_type option, const typename option_type::implicit_value_type value) -> auto& {
								std::invoke(self, option, value + typename option_type::default_value_type{});
								return self;
							},
							[](auto& self, const string_view_type option, const typename option_type::default_value_type value) -> auto& {
								std::invoke(self, option, value + typename option_type::implicit_value_type{});
								return self;
							},
							[](auto& self, const string_view_type option) -> auto& {
								std::invoke(self, option, typename option_type::value_type{});
								return self;
							},
					}};
		}

		[[nodiscard]] auto aliases() noexcept -> auto
		{
			return functional::y_combinator{
					[this](auto& self, const string_view_type alias_name, const string_view_type target_option_name) -> auto& {
						const auto option_names = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::parse_list(alias_name);
						if (not option_names.has_value()) { CommandLineOptionNameFormatError::panic(alias_name); }

						const auto option_view = *option_names;
						const auto option_size = std::ranges::distance(option_view);

						if (option_size != 1 and option_size != 2) { CommandLineOptionNameFormatError::panic(alias_name); }

						const auto o1 = string_view_type{*std::ranges::begin(option_view)};
						const auto o2 = option_size == 2
							                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
							                : string_view_type{};

						add_alias(o1, target_option_name);
						if (not o2.empty()) { add_alias(o2, target_option_name); }

						return self;
					}};
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		auto parse(const Begin begin, const End end) -> void
		{
			std::vector<string_view_type> unmatched{};

			std::ranges::for_each(
				begin,
				end,
				[this, &unmatched](const auto& string) -> void
				{
					const auto option = parse_option(string);

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
						if(std::ranges::equal(string_view_type{"--"}, string))
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
						const auto allocated_value = string_pool_.append(option->value);
						it->second->set_value(allocated_value);
					}
				});

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
					});

				std::cerr << std::format("Unrecognized option:\n {}", options);

				if (not allow_unrecognized_) { CommandLineOptionUnrecognizedError::panic(options); }
			}
		}

		template<std::ranges::input_range Range>
		auto parse(const Range& range) -> void
		{
			parse(std::ranges::begin(range), std::ranges::end(range));
		}

		auto parse() -> void
		{
			parse(platform::command_line_args());
		}

		auto contains(const string_view_type arg_name) const -> bool { return option_list_.contains(arg_name); }

		auto count(const string_view_type arg_name) const -> option_list_size_type { return option_list_.count(arg_name); }

		auto operator[](const string_view_type arg_name) const -> const option_type&
		{
			const auto it = option_list_.find(arg_name);
			if (it == option_list_.end()) { CommandLineOptionRequiredNotPresentError::panic(arg_name); }

			return *it->second;
		}
	};
}

#if defined(GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED)
#undef GAL_PROMETHEUS_INFRASTRUCTURE_COMMAND_LINE_PARSER_USE_EXPECTED
#endif
