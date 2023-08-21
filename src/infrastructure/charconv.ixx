// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:charconv;

import std;
import :type_traits;
import :runtime_error.exception;
import :runtime_error.terminate_message;

export namespace gal::prometheus::infrastructure
{
	[[nodiscard]] constexpr auto is_upper(const char c) noexcept -> bool
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return c >= 'A' and c <= 'Z'; }

		return std::isupper(c);
	}

	[[nodiscard]] constexpr auto is_upper(const std::basic_string_view<char> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto(*)(char c) noexcept -> bool>(is_upper)); }

	[[nodiscard]] constexpr auto is_lower(const char c) noexcept -> bool
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return c >= 'a' and c <= 'z'; }

		return std::islower(c);
	}

	[[nodiscard]] constexpr auto is_lower(const std::basic_string_view<char> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_lower)); }

	[[nodiscard]] constexpr auto is_alpha(const char c) noexcept -> bool
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return is_upper(c) or is_lower(c); }

		return std::isalpha(c);
	}

	[[nodiscard]] constexpr auto is_alpha(const std::basic_string_view<char> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_alpha)); }

	[[nodiscard]] constexpr auto is_digit(const char c) noexcept -> bool
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return c >= '0' and c <= '9'; }

		return std::isdigit(c);
	}

	[[nodiscard]] constexpr auto is_digit(const std::basic_string_view<char> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_digit)); }

	[[nodiscard]] constexpr auto is_alpha_digit(const char c) noexcept -> bool
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return is_alpha(c) or is_digit(c); }

		return std::isalnum(c);
	}

	[[nodiscard]] constexpr auto is_alpha_digit(const std::basic_string_view<char> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_alpha_digit)); }

	[[nodiscard]] constexpr auto to_upper(const char c) noexcept -> char
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return static_cast<char>(is_lower(c) ? c - 'a' + 'A' : c); }

		return static_cast<char>(std::toupper(c));
	}

	constexpr auto to_upper(std::basic_string<char>& string) noexcept -> void
	{
		std::ranges::for_each(
				string,
				[](char& c) noexcept -> void { c = to_upper(c); });
	}

	[[nodiscard]] constexpr auto to_upper(const std::basic_string_view<char> string) noexcept -> std::string
	{
		std::string result{};
		result.reserve(string.size());

		std::ranges::for_each(
				string,
				[&result](const char c) noexcept -> void { result.push_back(to_upper(c)); }
				);

		return result;
	}

	[[nodiscard]] constexpr auto to_lower(const char c) noexcept -> char
	{
		GAL_PROMETHEUS_IF_CONSTANT_EVALUATED { return static_cast<char>(is_upper(c) ? c - 'A' + 'a' : c); }

		return static_cast<char>(std::tolower(c));
	}

	constexpr auto to_lower(std::basic_string<char>& string) noexcept -> void
	{
		std::ranges::for_each(
				string,
				[](char& c) noexcept -> void { c = to_lower(c); });
	}

	[[nodiscard]] constexpr auto to_lower(const std::basic_string_view<char> string) noexcept -> std::string
	{
		std::string result{};
		result.reserve(string.size());

		std::ranges::for_each(
				string,
				[&result](const char c) noexcept -> void { result.push_back(to_lower(c)); });

		return result;
	}

	constexpr auto to_tittle(std::basic_string<char>& string) noexcept -> void
	{
		std::ranges::for_each(
				string,
				[first = true](char& c) mutable noexcept -> void
				{
					if (first)
					{
						c     = to_upper(c);
						first = false;
					}
					else if (c == ' ') { first = true; }
					else { c = to_lower(c); }
				});
	}

	[[nodiscard]] constexpr auto to_tittle(const std::basic_string_view<char> string) noexcept -> std::string
	{
		std::string result{};
		result.reserve(string.size());

		std::ranges::for_each(
				string,
				[&result, first = true](const char c) mutable noexcept -> void
				{
					char this_char;
					if (first)
					{
						this_char = to_upper(c);
						first     = false;
					}
					else if (c == ' ')
					{
						this_char = c;
						first     = true;
					}
					else { this_char = to_lower(c); }

					result.push_back(this_char);
				});

		return result;
	}

	// @see infrastructure.string.ixx
	// ------------------------------------------------------------
	// begin
	// ------------------------------------------------------------

	template<typename T, std::size_t N>
	struct basic_fixed_string;

	template<std::size_t N>
	[[nodiscard]] constexpr auto is_upper(const basic_fixed_string<char, N> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_upper)); }

	template<std::size_t N>
	[[nodiscard]] constexpr auto is_lower(const basic_fixed_string<char, N> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_lower)); }

	template<std::size_t N>
	[[nodiscard]] constexpr auto is_alpha(const basic_fixed_string<char, N> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_alpha)); }


	template<std::size_t N>
	[[nodiscard]] constexpr auto is_digit(const basic_fixed_string<char, N> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_digit)); }

	template<std::size_t N>
	[[nodiscard]] constexpr auto is_alpha_digit(const basic_fixed_string<char, N> string) noexcept -> bool { return std::ranges::all_of(string, static_cast<auto (*)(char c) noexcept -> bool>(is_alpha_digit)); }

	template<std::size_t N>
	[[nodiscard]] constexpr auto to_upper(const basic_fixed_string<char, N> string) noexcept -> basic_fixed_string<char, N>
	{
		basic_fixed_string<char, N> result{string};

		std::ranges::for_each(
				result,
				[](char& c) noexcept -> void { c = to_upper(c); });

		return result;
	}

	template<std::size_t N>
	[[nodiscard]] constexpr auto to_lower(const basic_fixed_string<char, N> string) noexcept -> basic_fixed_string<char, N>
	{
		basic_fixed_string<char, N> result{string};

		std::ranges::for_each(
				result,
				[](char& c) noexcept -> void { c = to_lower(c); });

		return result;
	}

	template<std::size_t N>
	[[nodiscard]] constexpr auto to_tittle(const basic_fixed_string<char, N> string) noexcept -> basic_fixed_string<char, N>
	{
		basic_fixed_string<char, N> result{string};

		std::ranges::for_each(
				result,
				[first = true](char& c) mutable noexcept -> void
				{
					if (first)
					{
						c     = to_upper(c);
						first = false;
					}
					else if (c == ' ') { first = true; }
					else { c = to_lower(c); }
				});

		return result;
	}

	// ------------------------------------------------------------
	// end
	// ------------------------------------------------------------

	template<traits::integral T, bool Throw = true>
	[[nodiscard]] constexpr auto from_string(const std::basic_string_view<char> string, int base = 10) noexcept(not Throw) -> std::conditional_t<Throw, T, std::optional<T>>
	{
		const auto begin = string.data();
		const auto end   = string.data() + string.size();

		T result;

		// fixme:
		// 	C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(154): error C2752: "std::tuple_size<_Ty>": more than one partial specialization matches the template argument list
		//	          with
		//	          [
		//	              _Ty=const std::from_chars_result
		//	          ]
		//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(604): note: maybe "std::tuple_size<const _Tuple>"
		//	  C:\Program Files\Microsoft Visual Studio\2022\Preview\VC\Tools\MSVC\14.38.32919\include\utility(604): note: or        "std::tuple_size<const _Tuple>"
		// const auto [last, error_code] = std::from_chars(begin, end, result, base);
		const auto from_chars_result = std::from_chars(begin, end, result, base);
		const auto last              = from_chars_result.ptr;
		const auto error_code        = from_chars_result.ec;

		if (error_code != std::errc{} or last != end
		)
		{
			if constexpr (Throw) { GAL_PROMETHEUS_RUNTIME_THROW_STRING_PARSE_ERROR("Can not convert string [{}] to integer", string); }
			else { return std::nullopt; }
		}

		return result;
	}

	template<traits::floating_point T, bool Throw = true>
	[[nodiscard]] constexpr auto from_string(const std::basic_string_view<char> string) noexcept(not Throw) -> std::conditional_t<Throw, T, std::optional<T>>
	{
		const auto begin = string.data();
		const auto end   = string.data() + string.size();

		T result;
		if (
			const auto [last, error_code] = std::from_chars(begin, end, result);
			error_code != std::errc{} or last != end)
		{
			if constexpr (Throw) { GAL_PROMETHEUS_RUNTIME_THROW_STRING_PARSE_ERROR("Can not convert string [{}] to integer", string); }
			else { return std::nullopt; }
		}

		return result;
	}
}
