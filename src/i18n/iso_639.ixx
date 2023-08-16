// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.i18n:iso_639;

import std;
import gal.prometheus.infrastructure;

export namespace gal::prometheus::i18n
{
	/**
	 * @brief ISO-639 language code.
	 *
	 * @note This class compresses this 2 or 3 character language code inside 15 bits.
	 * @note A 2 or 3 lower case language code selected from the following iso standards:
	 *	1. ISO 639-1 (2002)
	 *	2. ISO 639-2 (1998)
	 *	3. ISO 639-3 (2007)
	 *	4. ISO 639-5 (2008)
	 */
	// ReSharper disable once CppInconsistentNaming
	class [[nodiscard]] ISO639 final
	{
	public:
		// Encoded as follows:
		// [0:4]: the first letter
		// [5:9]: the second letter
		// [10:14]: the (optional) third letter
		// [15]: reserved
		using value_type = std::uint16_t;

		// for tuple
		using element_type = char;
		constexpr static std::size_t max_size{3};

		constexpr static auto parse(std::basic_string_view<element_type> string) noexcept -> std::optional<ISO639>;

	private:
		value_type value_;

		template<std::size_t Index>
		constexpr auto set(element_type c) noexcept -> void
		{
			constexpr auto shift = Index * 5;

			const auto value =
					static_cast<value_type>([c]
					{
						if (c >= 'a' and c <= 'z') { return c - 'a' + 1; }

						if (c >= 'A' and c <= 'Z') { return c - 'A' + 1; }

						if (c >= '1' and c <= '5') { return c - '1' + 27; }
					}());

			GAL_PROMETHEUS_DEBUG_ASSUME(value <= 0x1f);

			value_ &= ~(0x1f << shift);
			value_ |= value << shift;
		}

		constexpr explicit ISO639(const value_type value) noexcept
			: value_{value} {}

	public:
		// Since we can't just return "references" to bits of the value_type, we can't "modify" the data using structured-binding, we have to modify it separately.
		template<std::size_t Index>
		[[nodiscard]] constexpr auto get() const noexcept -> element_type
		{
			constexpr auto shift = Index * 5;

			const auto value = (value_ >> shift) & 0x1f;

			if (value == 0) { return 0; }

			if (value <= 26) { return 'a' + infrastructure::narrow_cast<element_type>(value - 1); }
			return '1' + infrastructure::narrow_cast<element_type>(value - 27);
		}

		[[nodiscard]] constexpr auto value() noexcept -> value_type& { return value_; }

		[[nodiscard]] constexpr auto value() const noexcept -> value_type { return value_; }

		[[nodiscard]] constexpr auto size() const noexcept -> std::size_t
		{
			const auto value = value_ & 0x7fff;

			if (value == 0) { return 0; }

			if (value <= 0x1f) { return 1; }

			if (value <= 0x3ff) { return 2; }

			return 3;
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool { return value() == 0; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept { return not empty(); }

		[[nodiscard]] constexpr auto code() const noexcept -> std::basic_string<element_type>;

		[[nodiscard]] friend constexpr auto operator==(const ISO639&, const ISO639&) noexcept -> bool = default;
		[[nodiscard]] friend constexpr auto operator<=>(const ISO639&, const ISO639&) noexcept        = default;
	};
}

export namespace std
{
	template<>
	struct tuple_size<gal::prometheus::i18n::ISO639> : integral_constant<std::size_t, gal::prometheus::i18n::ISO639::max_size> { };

	template<std::size_t Index>
	struct tuple_element<Index, gal::prometheus::i18n::ISO639>
	{
		using type = gal::prometheus::i18n::ISO639::element_type;
	};

	template<>
	struct hash<gal::prometheus::i18n::ISO639>
	{
		[[nodiscard]] auto operator()(const gal::prometheus::i18n::ISO639& i) const noexcept -> std::size_t { return hash<gal::prometheus::i18n::ISO639::value_type>{}(i.value()); }
	};
}

export namespace gal::prometheus::i18n
{
	constexpr auto ISO639::parse(const std::basic_string_view<element_type> string) noexcept -> std::optional<ISO639>
	{
		// GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
		// 		string.size() == 2 or string.size() == 3,
		// 		"ISO-639 incorrect length, got '{}'", string.size());
		if (not(string.size() == 2 or string.size() == 3)) { return std::nullopt; }

		// std::ranges::for_each(
		// 		string,
		// 		[](const auto c) -> void
		// 		{
		// 			GAL_PROMETHEUS_RUNTIME_ASSUME_OR_THROW_STRING_PARSE_ERROR(
		// 					(c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '1' and c <= '5'),
		// 					"Must be letters or the digits between '1' and '5', got '{}'",
		// 					c);
		// 		});

		if (std::ranges::all_of(string, [](const auto c) noexcept -> bool { return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '1' and c <= '5'); }))
		{
			ISO639 result{0};
			result.set<0>(string[0]);
			result.set<1>(string[1]);
			if (string.size() > 2) { result.set<3>(string[3]); }
			return result;
		}

		return std::nullopt;
	}

	constexpr auto ISO639::code() const noexcept -> std::basic_string<element_type>
	{
		const auto [v1, v2, v3] = *this;

		std::basic_string<element_type> result{};
		result.push_back(v1);
		result.push_back(v2);
		result.push_back(v3);

		return result;
	}
}
