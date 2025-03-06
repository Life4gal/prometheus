// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <command_line_parser/regex.hpp>

namespace gal::prometheus::clp
{
	template<regex::string_type StringType = std::basic_string<regex::char_type>>
	class CommandLineOptionParser;

	template<regex::string_type StringType, regex::string_type StringViewType>
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
				regex::parser::parse<T>(std::declval<string_view_type>());
			}
		[[nodiscard]] constexpr auto as() const -> std::optional<T>
		{
			if (not set())
			{
				return std::nullopt;
			}

			if constexpr (
				std::is_same_v<
					std::remove_cvref_t<decltype(regex::parser::parse<T>(std::declval<string_view_type>()))>,
					T
				>
			)
			{
				return regex::parser::parse<T>(current_value_);
			}
			else
			{
				auto value = regex::parser::parse<T>(current_value_);
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
}
