#pragma once

// numeric::random
#include <numeric/numeric.hpp>
// i18n::range
#include <i18n/i18n.hpp>

namespace gen_detail
{
	[[nodiscard]] inline auto ranges_ascii() noexcept -> auto
	{
		using namespace gal::prometheus::i18n;

		return RangeBuilder{}.ascii().range();
	}

	[[nodiscard]] inline auto ranges_latin() noexcept -> auto
	{
		using namespace gal::prometheus::i18n;

		return RangeBuilder{}.latin().range();
	}

	[[nodiscard]] inline auto ranges_all() noexcept -> auto
	{
		using namespace gal::prometheus::i18n;

		return RangeBuilder{}.greek().korean().japanese().simplified_chinese_common().range();
	}

	[[nodiscard]] auto generate_char_32(auto& random, const std::uint32_t from, const std::uint32_t to) noexcept -> std::uint32_t
	{
		std::uint32_t v;
		do
		{
			v = random.template get<std::uint32_t>(from, to);
		} while (
			// skip surrogate pairs
			(v >= 0xd800 and v <= 0xdfff) or
			// skip non-characters
			(v >= 0xfdd0 and v <= 0xfdef) or ((v & 0xfffe) == 0xfffe) or
			// skip control characters
			(v <= 0x001f) or (v >= 0x007f and v <= 0x009f) or
			// skip special characters
			(v >= 0xfff0 and v <= 0xfff8) or
			// skip tag characters and variation selector supplement
			(v >= 0x000e'0000 and v <= 0x000e'01ef) or
			// skip private use areas
			(v >= 0x000f'0000) or
			// skip ideographic description characters
			(v >= 0x2ff0 and v <= 0x2fff)
		);

		return v;
	}

	template<typename Char>
	[[nodiscard]] auto make_random_string(const auto& generator, std::size_t length) noexcept -> std::basic_string<Char>
	{
		std::basic_string<Char> result{};
		result.reserve(length);

		if (length != 0)
		{
			while (length -= generator(result, length)) {}
		}

		return result;
	}

	template<typename Char, bool AsciiOnly = false>
	[[nodiscard]] auto make_random_latin_string(const std::size_t min_length, const std::size_t max_length) noexcept -> std::basic_string<Char>
	{
		using namespace gal::prometheus::numeric;

		using char_type = Char;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto ranges = []() noexcept
		{
			if constexpr (AsciiOnly)
			{
				return ranges_ascii();
			}
			else
			{
				return ranges_latin();
			}
		}();

		const auto g = [&random, &ranges](auto& dest, [[maybe_unused]] const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			const auto v = random.get(from, to);
			const auto v1 = static_cast<char_type>(v);
			dest.push_back(v1);
			return 1;
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::make_random_string<char_type>(g, length);
	}

	template<typename Char>
	[[nodiscard]] auto make_random_utf8_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<Char>
	{
		using namespace gal::prometheus::numeric;

		using char_type = Char;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto ranges = ranges_all();

		const auto g = [&random, ranges](auto& dest, const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			std::size_t max_try = 3;
			while (max_try--)
			{
				const auto v = generate_char_32(random, from, to);

				if (v < 0x0080)
				{
					const auto v1 = static_cast<char_type>(v);
					dest.push_back(v1);
					return 1;
				}

				if (remaining >= 2 and v < 0x0800)
				{
					const auto v1 = static_cast<char_type>(0xc0 | (v >> 6));
					const auto v2 = static_cast<char_type>(0x80 | (v & 0x3f));
					dest.push_back(v1);
					dest.push_back(v2);
					return 2;
				}

				if (remaining >= 3 and v < 0x0001'0000)
				{
					const auto v1 = static_cast<char_type>(0xe0 | (v >> 12));
					const auto v2 = static_cast<char_type>(0x80 | ((v >> 6) & 0x3f));
					const auto v3 = static_cast<char_type>(0x80 | (v & 0x3f));
					dest.push_back(v1);
					dest.push_back(v2);
					dest.push_back(v3);
					return 3;
				}

				if (remaining >= 4)
				{
					const auto v1 = static_cast<char_type>(0xf0 | (v >> 18));
					const auto v2 = static_cast<char_type>(0x80 | ((v >> 12) & 0x3f));
					const auto v3 = static_cast<char_type>(0x80 | ((v >> 6) & 0x3f));
					const auto v4 = static_cast<char_type>(0x80 | (v & 0x3f));
					dest.push_back(v1);
					dest.push_back(v2);
					dest.push_back(v3);
					dest.push_back(v4);
					return 4;
				}
			}

			if constexpr (std::is_same_v<Char, char>)
			{
				dest.push_back('?');
			}
			else
			{
				dest.push_back(u8'?');
			}
			return 1;
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::make_random_string<char_type>(g, length);
	}

	template<bool Little>
	[[nodiscard]] auto make_random_utf16_string(const std::size_t min_length, const std::size_t max_length) noexcept -> std::basic_string<char16_t>
	{
		using namespace gal::prometheus::numeric;

		using char_type = char16_t;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto ranges = ranges_all();

		const auto g = [&random, &ranges](auto& dest, const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			std::size_t max_try = 3;
			while (max_try--)
			{
				const auto v = generate_char_32(random, from, to);

				if (v < 0xffff)
				{
					if constexpr (Little)
					{
						const auto v1 = static_cast<char_type>(v);
						dest.push_back(v1);
					}
					else
					{
						const auto v1 = static_cast<char_type>(v);
						const auto v1_swap = std::byteswap(v1);
						dest.push_back(static_cast<char_type>(v1_swap));
					}

					return 1;
				}

				if (remaining >= 2)
				{
					const auto vv = v - 0x0001'0000;
					if constexpr (Little)
					{
						const auto v1 = static_cast<char_type>(0xd800 | ((vv >> 10) & 0x03ff));
						const auto v2 = static_cast<char_type>(0xdc00 | (vv & 0x03ff));
						dest.push_back(v1);
						dest.push_back(v2);
					}
					else
					{
						const auto v1 = static_cast<char_type>(0xd800 | ((v >> 10) & 0x03ff));
						const auto v1_swap = std::byteswap(v1);
						const auto v2 = static_cast<char_type>(0xdc00 | (v & 0x03ff));
						const auto v2_swap = std::byteswap(v2);
						dest.push_back(v1_swap);
						dest.push_back(v2_swap);
					}
					return 2;
				}
			}

			dest.push_back(u'?');
			return 1;
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::make_random_string<char_type>(g, length);
	}

	[[nodiscard]] inline auto make_random_utf32_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
	{
		using namespace gal::prometheus::numeric;

		using char_type = char32_t;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto ranges = ranges_all();

		const auto g = [&random, &ranges](auto& dest, [[maybe_unused]] const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			const auto v = generate_char_32(random, from, to);

			const auto v1 = static_cast<char_type>(v);
			dest.push_back(v1);
			return 1;
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::make_random_string<char_type>(g, length);
	}
}

// ==============================
// LATIN

[[nodiscard]] inline auto make_random_ascii_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_latin_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char>(min_length, max_length);
}

// ==============================
// UTF8

[[nodiscard]] inline auto make_random_utf8_char_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_utf8_string<char>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_char_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_char_string_latin_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char8_t>
{
	return gen_detail::make_random_utf8_string<char8_t>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char8_t>
{
	return gen_detail::make_random_latin_string<char8_t, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_string_latin_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char8_t>
{
	return gen_detail::make_random_latin_string<char8_t>(min_length, max_length);
}

// ==============================
// UTF16

[[nodiscard]] inline auto make_random_utf16_le_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	return gen_detail::make_random_utf16_string<true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf16_be_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	return gen_detail::make_random_utf16_string<false>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf16_le_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	return gen_detail::make_random_latin_string<char16_t, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf16_le_string_latin_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	return gen_detail::make_random_latin_string<char16_t>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf16_be_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	auto result = gen_detail::make_random_latin_string<char16_t, true>(min_length, max_length);
	std::ranges::for_each(
		result,
		[](char16_t& c) noexcept -> void
		{
			c = std::byteswap(c);
		}
	);
	return result;
}

[[nodiscard]] inline auto make_random_utf16_be_string_latin_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	auto result = gen_detail::make_random_latin_string<char16_t>(min_length, max_length);
	std::ranges::for_each(
		result,
		[](char16_t& c) noexcept -> void
		{
			c = std::byteswap(c);
		}
	);
	return result;
}

// ==============================
// UTF32

[[nodiscard]] inline auto make_random_utf32_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
{
	return gen_detail::make_random_utf32_string(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf32_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
{
	return gen_detail::make_random_latin_string<char32_t>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf32_string_latin_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
{
	return gen_detail::make_random_latin_string<char32_t>(min_length, max_length);
}
