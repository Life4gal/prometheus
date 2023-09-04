// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>
#include <cstdlib>

export module gal.prometheus.infrastructure:command_line_parser;

import std;
import :error.exception;
import :error.debug;
import :compiler;
import :type_traits;
import :string_pool;

namespace gal::prometheus::infrastructure
{
	using regex_char_type = char;

	template<typename Range>
	concept regex_string_type = std::ranges::contiguous_range<Range> and std::is_same_v<std::ranges::range_value_t<Range>, regex_char_type>;

	using regex_type = std::basic_regex<regex_char_type>;
	template<regex_string_type Range>
	using regex_match_result_type = std::match_results<typename Range::const_iterator>;
	template<regex_string_type Range>
	using regex_token_iterator = std::regex_token_iterator<typename Range::const_iterator>;

	template<regex_string_type Range>
	auto make_regex(const Range& range) -> regex_type { return regex_type{std::ranges::data(range), std::ranges::size(range)}; }

	template<regex_string_type Range>
	auto regex_match(const Range& range, regex_match_result_type<Range>& result, const regex_type& regex) -> bool { return std::regex_match(std::ranges::begin(range), std::ranges::end(range), result, regex); }

	// ReSharper disable StringLiteralTypo
	// ReSharper disable CppTemplateArgumentsCanBeDeduced
	#define LIST_SEPARATOR_CHAR ','
	#define LIST_SEPARATOR_CHARS ","
	constexpr regex_char_type                         list_separator_char{LIST_SEPARATOR_CHAR};
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
	constexpr std::basic_string_view<regex_char_type> pattern_list{"([[:alnum:]][-_[:alnum:]\\.]*)(" LIST_SEPARATOR_CHARS "\\s*[[:alnum:]][-_[:alnum:]]*)*"};
	constexpr std::basic_string_view<regex_char_type> pattern_list_separator{LIST_SEPARATOR_CHARS "\\s*"};
	// ReSharper restore CppTemplateArgumentsCanBeDeduced
	// ReSharper restore StringLiteralTypo

	export
	{
		class CommandLineOptionArgumentTypeError final : public StringParseError
		{
		public:
			CommandLineOptionArgumentTypeError(
					const std::string_view text,
					const std::string_view required_type
					)
				: StringParseError{std::format("Cannot parse `{}` as `{}`.", text, required_type)} {}
		};

		class CommandLineOptionNameFormatError final : public StringParseError
		{
		public:
			explicit CommandLineOptionNameFormatError(const std::string_view option)
				: StringParseError{std::format("Cannot parse `{}` as option name", option)} {}
		};

		class CommandLineOptionAlreadyExistsError final : public InvalidArgumentError
		{
		public:
			explicit CommandLineOptionAlreadyExistsError(const std::string_view option)
				: InvalidArgumentError{std::format("Option `{}` already exists!", option)} {}
		};

		class CommandLineOptionUnrecognizedError final : public OutOfRangeError
		{
		public:
			explicit CommandLineOptionUnrecognizedError(const std::string_view option)
				: OutOfRangeError{std::format("Unrecognized option:\n {}", option)} {}
		};

		class CommandLineOptionRequiredNotPresentError final : public OutOfRangeError
		{
		public:
			explicit CommandLineOptionRequiredNotPresentError(const std::string_view option)
				: OutOfRangeError{std::format("Required option `{}` not present", option)} {}
		};

		class CommandLineOptionRequiredNotSetError final : public StringParseError
		{
		public:
			explicit CommandLineOptionRequiredNotSetError(const std::string_view option)
				: StringParseError{std::format("Required option `{}` not set and no default value present", option)} {}
		};
	}

	using descriptor_boolean = bool;

	struct descriptor_integer
	{
		enum class Base
		{
			BINARY,
			OCTAL,
			DECIMAL,
			HEXADECIMAL,
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

	using descriptor_comma_list = std::vector<std::basic_string<regex_char_type>>;

	template<bool True, regex_string_type Range>
	[[nodiscard]] auto parse_boolean(const Range& range) -> descriptor_boolean
	{
		const static auto regex_true  = make_regex(pattern_boolean_true);
		const static auto regex_false = make_regex(pattern_boolean_false);

		regex_match_result_type<Range> result;
		if constexpr (True) { regex_match(range, result, regex_true); }
		else { regex_match(range, result, regex_false); }
		return not result.empty();
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_integer(const Range& range) -> descriptor_integer
	{
		const static auto regex = make_regex(pattern_integer);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 3) { throw CommandLineOptionArgumentTypeError{range, compiler::type_name<descriptor_integer>()}; }

		const auto is_negative = result[1] == '-';

		const auto&                  sub = result[2];
		const std::basic_string_view sub_string{sub.first, sub.second};
		if (sub_string.starts_with("0b"))
		{
			return {
					.is_negative = is_negative,
					.base = descriptor_integer::Base::BINARY,
					.value = sub_string.substr(2)};
		}
		if (sub_string.starts_with("0x"))
		{
			return {
					.is_negative = is_negative,
					.base = descriptor_integer::Base::HEXADECIMAL,
					.value = sub_string.substr(2)};
		}
		if (sub_string.starts_with('0'))
		{
			return {
					.is_negative = is_negative,
					.base = descriptor_integer::Base::OCTAL,
					.value = sub_string.substr(1)};
		}
		return {
				.is_negative = is_negative,
				.base = descriptor_integer::Base::DECIMAL,
				.value = sub_string};
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_option(const Range& range) -> descriptor_option
	{
		const static auto regex = make_regex(pattern_option);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 5) { throw CommandLineOptionArgumentTypeError{range, compiler::type_name<descriptor_option>()}; }

		if (result.length(4) > 0)
		{
			return {
					.name = {result[4].first, result[4].second},
					.value = ""};
		}

		return {
				.name = {result[1].first, result[1].second},
				.value = {result[3].first, result[3].second}};
	}

	template<regex_string_type Range>
	[[nodiscard]] auto parse_list(const Range& range) -> auto//descriptor_comma_list
	{
		const static auto regex           = make_regex(pattern_list);
		const static auto regex_separator = make_regex(pattern_list_separator);

		regex_match_result_type<Range> result;
		regex_match(range, result, regex);
		if (result.size() != 3) { throw CommandLineOptionArgumentTypeError{range, compiler::type_name<descriptor_comma_list>()}; }

		regex_token_iterator<Range> token_iterator{result[0].first, result[0].second, regex_separator, -1};

		// return std::ranges::to<descriptor_comma_list>(
		// 		std::ranges::subrange{token_iterator, regex_token_iterator<Range>{}} |
		// 		std::views::transform([](const auto& sm) { return descriptor_comma_list::value_type{sm}; }));
		return std::ranges::subrange{token_iterator, regex_token_iterator<Range>{}} |
				std::views::transform([](const auto& sm) { return std::basic_string_view<regex_char_type>{sm.first, sm.second}; }) |
				std::views::common;
	}

	struct string_parse_functor
	{
		// boolean
		template<typename T, regex_string_type Range>
			requires std::is_same_v<T, bool>
		[[nodiscard]] auto operator()(const Range& range) -> T
		{
			if (parse_boolean<true>(range)) { return true; }

			if (parse_boolean<false>(range)) { return false; }

			throw CommandLineOptionArgumentTypeError{range, compiler::type_name<bool>()};
		}

		// integral
		// not bool, avoid ambiguous
		template<traits::integral T, regex_string_type Range>
			requires(not std::is_same_v<T, bool>)
		[[nodiscard]] auto operator()(const Range& range) -> T
		{
			const auto& [is_negative, base, value] = parse_integer(range);
			const auto  base_num                   = [](const descriptor_integer::Base b)noexcept -> int
			{
				if (b == descriptor_integer::Base::BINARY) { return 2; }
				if (b == descriptor_integer::Base::OCTAL) { return 8; }
				if (b == descriptor_integer::Base::DECIMAL) { return 10; }
				if (b == descriptor_integer::Base::HEXADECIMAL) { return 16; }
				GAL_PROMETHEUS_DEBUG_UNREACHABLE("Unknown base");
			}(base);

			using type = std::make_unsigned_t<T>;
			type result;
			if (
				const auto [last, error] = std::from_chars(value.data(), value.data() + value.size(), result, base_num);
				error != std::errc{} or last != value.data() + value.size()
			) { throw CommandLineOptionArgumentTypeError{range, compiler::type_name<type>()}; }

			if (is_negative)
			{
				if constexpr (not std::numeric_limits<T>::is_signed) { throw CommandLineOptionArgumentTypeError{range, compiler::type_name<type>()}; }

				return static_cast<T>(-static_cast<T>(result - 1) - 1);
			}

			return result;
		}

		// list
		// not string, avoid ambiguous
		template<std::ranges::range OutRange, regex_string_type Range>
			requires(not std::is_constructible_v<OutRange, Range>) and
					requires
					{
						std::declval<string_parse_functor&>().operator()<std::ranges::range_value_t<OutRange>>(std::declval<std::ranges::range_const_reference_t<std::remove_cvref_t<decltype(parse_list(std::declval<const Range&>()))>>>());
					}
		auto operator()(const Range& range, OutRange& out) -> void
		{
			const auto list = parse_list(range);

			if constexpr (requires { out.reserve(list.size()); }) { out.reserve(list.size() + out.size()); }

			std::ranges::for_each(
					list,
					[this, &out](const auto& string) -> void//
					{
						if constexpr (requires { out.emplace_back(this->operator()<std::ranges::range_value_t<OutRange>>(string)); })//
						{
							out.emplace_back(this->operator()<std::ranges::range_value_t<OutRange>>(string));
						}
						else if constexpr (requires { out.push_back(this->operator()<std::ranges::range_value_t<OutRange>>(string)); })//
						{
							out.push_back(this->operator()<std::ranges::range_value_t<OutRange>>(string));
						}
						else if constexpr (requires { out.emplace(this->operator()<std::ranges::range_value_t<OutRange>>(string)); })//
						{
							out.emplace(this->operator()<std::ranges::range_value_t<OutRange>>(string));
						}
						else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
					});
		}

		// list
		// not string, avoid ambiguous
		template<std::ranges::range OutRange, regex_string_type Range>
			requires(not std::is_constructible_v<OutRange, Range>) and
					requires
					{
						std::declval<string_parse_functor&>().operator()(std::declval<const Range&>(), std::declval<OutRange&>());
					}
		[[nodiscard]] auto operator()(const Range& range) -> OutRange
		{
			OutRange result{};

			this->operator()(range, result);

			return result;
		}

		// string / construct from string
		// not list, avoid ambiguous
		template<typename T, regex_string_type Range>
			requires std::is_constructible_v<T, Range>
		[[nodiscard]] auto operator()(const Range& range) noexcept(std::is_nothrow_constructible_v<T, Range>) -> T { return T{range}; }
	};

	#if GAL_PROMETHEUS_COMPILER_MSVC
	const static int          g_argc = *__p___argc();
	static const char* const* g_argv = *__p___argv();
	#else
	static int		  g_argc = 0;
	static char* const* g_argv = nullptr;

	__attribute__((constructor)) auto do_read_command_line(const int		   argc, const char* argv[])  -> void
	{
		g_argc = argc;
		g_argv = argv;
	}
	#endif

	export
	{
		template<regex_string_type StringType, regex_string_type StringViewType>
		class CommandLineOption;
		template<regex_string_type StringType, regex_string_type StringViewType>
		class CommandLineOptionAppender;
		template<regex_string_type StringType = std::basic_string<regex_char_type>>
		class CommandLineOptionParser;

		template<regex_string_type StringType, regex_string_type StringViewType>
		class CommandLineOption
		{
		public:
			using string_type = StringType;
			using string_view_type = StringViewType;

			using appender_type = CommandLineOptionAppender<string_type, string_view_type>;
			using parser_type = CommandLineOptionParser<string_type>;

			friend appender_type;
			friend parser_type;

		private:
			struct value_type;

			struct implicit_value_type;

			struct default_value_type
			{
				string_view_type value;

				[[nodiscard]] constexpr auto operator+(const implicit_value_type& value) const noexcept -> value_type { return {*this, value}; }

				[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept { return value; }
			};

			struct implicit_value_type
			{
				string_view_type value;

				[[nodiscard]] constexpr auto operator+(const default_value_type& value) const noexcept -> value_type { return {value, *this}; }

				[[nodiscard]] constexpr explicit(false) operator string_view_type() const noexcept { return value; }
			};

			struct value_type
			{
				default_value_type  dv;
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
			string_view_type description_;
			string_view_type help_if_error_;

			value_type       value_;
			string_view_type current_value_;

			constexpr auto set_value_default() noexcept -> void { current_value_ = value_.dv; }

			constexpr auto set_value_implicit() noexcept -> void { current_value_ = value_.iv; }

			constexpr auto set_value(const string_view_type value = secret_value) noexcept -> void { current_value_ = value; }

			template<typename AllocateFunction>
				requires std::is_same_v<std::invoke_result_t<AllocateFunction, string_view_type>, string_view_type>
			constexpr auto reborn(AllocateFunction allocator) -> void
			{
				option_short_format_ = allocator(option_short_format_);
				option_long_format_  = allocator(option_long_format_);
				description_         = allocator(description_);
				help_if_error_       = allocator(help_if_error_);

				if (not value_.dv.value.empty()) { value_.dv = {allocator(value_.dv.value)}; }
				if (not value_.iv.value.empty()) { value_.iv = {allocator(value_.iv.value)}; }

				set_value_implicit();
			}

		public:
			constexpr CommandLineOption(
					const string_view_type option_short_format,
					const string_view_type option_long_format,
					const string_view_type description,
					const value_type       value,
					const string_view_type help_if_error) noexcept
				: option_short_format_{option_short_format},
				option_long_format_{option_long_format},
				description_{description},
				help_if_error_{help_if_error},
				value_{value},
				// `Defaults` use implicit_value, if the option is given `explicitly` the `default_value` is used, if the option is given `explicitly` and a value is specified the specified value is used.
				current_value_{value_.iv} { }

			[[nodiscard]] constexpr auto set() const noexcept -> bool { return not current_value_.empty(); }

			[[nodiscard]] constexpr auto has_default() const noexcept -> bool { return not value_.combination_value_.dv.empty(); }

			[[nodiscard]] constexpr auto default_value() const noexcept -> string_view_type { return value_.combination_value_.dv; }

			[[nodiscard]] constexpr auto is_default() const noexcept -> bool { return current_value_ == default_value(); }

			[[nodiscard]] constexpr auto has_implicit() const noexcept -> bool { return not value_.combination_value_.iv.empty(); }

			[[nodiscard]] constexpr auto implicit_value() const noexcept -> string_view_type { return value_.combination_value_.iv; }

			[[nodiscard]] constexpr auto is_implicit() const noexcept -> bool { return current_value_ == implicit_value(); }

			template<typename T>
				requires requires
				{
					std::declval<string_parse_functor&>().operator()<T>(std::declval<string_view_type>());
				}
			[[nodiscard]] auto as() const -> T
			{
				if (not set()) { throw CommandLineOptionRequiredNotSetError{option_short_format_.empty() ? option_long_format_ : option_short_format_}; }

				return string_parse_functor{}.operator()<T>(current_value_);
			}

			[[nodiscard]] auto help() const -> string_type
			{
				(void)this;
				GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();

				return {};
			}
		};

		template<regex_string_type StringType, regex_string_type StringViewType>
		class CommandLineOptionAppender
		{
		public:
			using string_type = StringType;
			using string_view_type = StringViewType;

			using option_type = CommandLineOption<string_type, string_view_type>;
			using parser_type = CommandLineOptionParser<string_type>;

			using value_type = typename option_type::value_type;
			using default_value_type = typename option_type::default_value_type;
			using implicit_value_type = typename option_type::implicit_value_type;

			friend parser_type;

		private:
			std::reference_wrapper<parser_type> parser_;

			explicit CommandLineOptionAppender(
					std::reference_wrapper<parser_type> parser
					) noexcept
				: parser_{parser} {}

		public:
			auto operator()(
					string_view_type option,
					string_view_type description,
					value_type       value         = value_type{},
					string_view_type help_if_error = {}) -> CommandLineOptionAppender&;

			auto operator()(
					const string_view_type   option,
					const string_view_type   description,
					const default_value_type default_value,
					const string_view_type   help_if_error = {}
					) -> CommandLineOptionAppender& { return this->operator()(option, description, default_value + implicit_value_type{}, help_if_error); }

			auto operator()(
					const string_view_type    option,
					const string_view_type    description,
					const implicit_value_type default_value,
					const string_view_type    help_if_error = {}) -> CommandLineOptionAppender& { return this->operator()(option, description, default_value + default_value_type{}, help_if_error); }
		};

		template<regex_string_type StringType>
		class CommandLineOptionParser
		{
		public:
			using string_type = StringType;
			using string_pool_type = StringPool<typename string_type::value_type, false>;
			using string_view_type = typename string_pool_type::view_type;

			using option_type = CommandLineOption<string_type, string_view_type>;
			using appender_type = CommandLineOptionAppender<string_type, string_view_type>;

			using option_list_type = std::unordered_map<string_view_type, option_type>;

			using list_size_type = typename option_list_type::size_type;

			friend appender_type;

			[[nodiscard]] constexpr static auto default_value(const string_view_type value) noexcept -> typename option_type::default_value_type { return option_type::default_value(value); }

			[[nodiscard]] constexpr static auto implicit_value(const string_view_type value) noexcept -> typename option_type::implicit_value_type { return option_type::implicit_value(value); }

		private:
			string_pool_type string_pool_;

			string_view_type this_program_;
			string_view_type about_this_program_;

			option_list_type option_list_;

			bool allow_unrecognized_;

			auto do_add_option(const string_view_type name, option_type option) -> void
			{
				const auto [_, inserted] = option_list_.emplace(name, option);
				if (not inserted) { throw CommandLineOptionAlreadyExistsError{name}; }
			}

			auto add_option(option_type option) -> void
			{
				option.reborn([this](const string_view_type value) -> string_view_type { return string_pool_.append(value); });

				if (not option.option_short_format_.empty()) { do_add_option(option.option_short_format_, option); }
				do_add_option(option.option_long_format_, option);
			}

		public:
			CommandLineOptionParser(const string_view_type this_program, const string_view_type about_this_program, const bool allow_unrecognized = false)
				: this_program_{this_program},
				about_this_program_{about_this_program},
				allow_unrecognized_{allow_unrecognized} {}

			[[nodiscard]] auto options() noexcept -> appender_type { return appender_type{*this}; }

			auto parse(const std::span<const string_view_type> args) -> void
			{
				std::vector<string_view_type> unmatched{};

				std::ranges::for_each(
						args.begin(),
						args.end(),
						[this, &unmatched](const auto& string) -> void
						{
							const auto option = parse_option(string);

							auto it = option_list_.find(option.name);
							if (it == option_list_.end())
							{
								unmatched.emplace_back(option.name);
								return;
							}

							auto another_it = option_list_.find(option.name == it->second.option_short_format_ ? it->second.option_long_format_ : it->second.option_short_format_);

							// Since we allow `--option`, we are not sure here whether the string is `--option` or `--option`.
							if (option.value.empty())
							{
								if (std::ranges::starts_with(string, string_view_type{"--"}))
								{
									// --option
									it->second.set_value_default();

									if (another_it != option_list_.end()) { another_it->second.set_value_default(); }
								}
								else
								{
									// -option
									it->second.set_value();

									if (another_it != option_list_.end()) { another_it->second.set_value(); }
								}
							}
							else
							{
								const auto allocated_value = string_pool_.append(option.value);
								it->second.set_value(allocated_value);

								if (another_it != option_list_.end()) { another_it->second.set_value(allocated_value); }
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

					std::cerr << options;

					if (not allow_unrecognized_) { throw CommandLineOptionUnrecognizedError{options}; }
				}
			}

			auto parse() -> void { return parse(std::span<const string_view_type>{g_argv, g_argc}); }

			[[nodiscard]] auto help() const -> string_type
			{
				(void)this;
				GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();

				return {};
			}

			auto contains(const string_view_type arg_name) const -> bool { return option_list_.contains(arg_name); }

			auto count(const string_view_type arg_name) const -> list_size_type { return option_list_.count(arg_name); }

			auto operator[](const string_view_type arg_name) const -> const option_type&
			{
				const auto it = option_list_.find(arg_name);
				if (it == option_list_.end()) { throw CommandLineOptionRequiredNotPresentError{arg_name}; }

				return it->second;
			}
		};

		template<regex_string_type StringType, regex_string_type StringViewType>
		auto CommandLineOptionAppender<StringType, StringViewType>::operator()(
				string_view_type option,
				string_view_type description,
				value_type       value,
				string_view_type help_if_error)
			-> CommandLineOptionAppender&
		{
			const auto option_names = parse_list(option);
			const auto option_size  = std::ranges::distance(option_names);

			if (option_size != 1 and option_size != 2) { throw CommandLineOptionNameFormatError{option}; }

			const auto o1 = string_view_type{*std::ranges::begin(option_names)};
			const auto o2 = option_size == 2 ? string_view_type{*std::ranges::next(std::ranges::begin(option_names), 1)} : string_view_type{};

			auto op = (o1.size() < o2.size()) ? option_type{o1, o2, description, value, help_if_error} : option_type{o2, o1, description, value, help_if_error};
			parser_.get().add_option(op);

			return *this;
		}
	}
}
