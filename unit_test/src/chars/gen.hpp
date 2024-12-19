#pragma once

// numeric::random
#include <numeric/numeric.hpp>
// i18n::range
#include <i18n/i18n.hpp>

namespace gen_detail
{
	// =====================================================

	[[nodiscard]] inline auto ranges_ascii() noexcept -> const auto&
	{
		using namespace gal::prometheus::i18n;

		const static auto range = RangeBuilder{}.ascii().range();

		return range;
	}

	[[nodiscard]] inline auto ranges_latin() noexcept -> const auto&
	{
		using namespace gal::prometheus::i18n;

		const static auto range = RangeBuilder{}.latin().range();

		return range;
	}

	[[nodiscard]] inline auto ranges_all() noexcept -> const auto&
	{
		using namespace gal::prometheus::i18n;

		const static auto range = RangeBuilder{}.latin().greek().korean().japanese().simplified_chinese_common().range();

		return range;
	}

	template<typename Char>
	[[nodiscard]] auto generate_string(const auto& generator, std::size_t length) noexcept -> std::basic_string<Char>
	{
		std::basic_string<Char> result{};
		result.reserve(length);

		if (length != 0)
		{
			while (length -= generator(result, length)) {}
		}

		return result;
	}

	// =====================================================
	// LATIN

	template<typename Char, bool AsciiOnly = false>
	[[nodiscard]] auto make_random_latin_string(const std::size_t min_length, const std::size_t max_length) noexcept -> std::basic_string<Char>
	{
		using namespace gal::prometheus::numeric;

		using char_type = Char;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto& ranges = []() noexcept
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

			{
				const auto v1 = static_cast<char_type>(v);
				dest.push_back(v1);
				return 1;
			}
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::generate_string<char_type>(g, length);
	}

	// =====================================================
	// UTF8

	template<typename Char, bool AsciiOnly = false>
	[[nodiscard]] auto make_random_utf8_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<Char>
	{
		using namespace gal::prometheus::numeric;

		using char_type = Char;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto& ranges = []() noexcept
		{
			if constexpr (AsciiOnly)
			{
				return ranges_ascii();
			}
			else
			{
				return ranges_all();
			}
		}();

		const auto g = [&random, ranges](auto& dest, [[maybe_unused]] const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			std::size_t max_try = 3;
			while (max_try--)
			{
				auto v = random.get<std::uint32_t>(from, to);

				if (v < 0x80)
				{
					const auto v1 = static_cast<char_type>(v);
					dest.push_back(v1);

					return 1;
				}

				if constexpr (not AsciiOnly)
				{
					if (remaining >= 2 and v < 0x800)
					{
						const auto v1 = static_cast<char_type>(0xc0 | ((v >> 6) & 0x1f));
						const auto v2 = static_cast<char_type>(0x80 | (v & 0x3f));
						dest.push_back(v1);
						dest.push_back(v2);

						return 2;
					}

					if (v >= 0xd800 and v <= 0xdfff)
					{
						// skip surrogate pairs
						v += (0xdfff - 0xd800 + 1);
					}

					if (remaining >= 3 and v < 0x1'0000)
					{
						const auto v1 = static_cast<char_type>(0xe0 | ((v >> 12) & 0xf));
						const auto v2 = static_cast<char_type>(0x80 | ((v >> 6) & 0x3f));
						const auto v3 = static_cast<char_type>(0x80 | (v & 0x3f));
						dest.push_back(v1);
						dest.push_back(v2);
						dest.push_back(v3);

						return 3;
					}

					if (remaining >= 4)
					{
						const auto v1 = static_cast<char_type>(0xf0 | ((v >> 18) & 0x7));
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
		return gen_detail::generate_string<char_type>(g, length);
	}

	// =====================================================
	// UTF16

	template<bool Little, bool AsciiOnly = false>
	[[nodiscard]] auto make_random_utf16_string(const std::size_t min_length, const std::size_t max_length) noexcept -> std::basic_string<char16_t>
	{
		using namespace gal::prometheus::numeric;

		using char_type = char16_t;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto& ranges = []() noexcept
		{
			if constexpr (AsciiOnly)
			{
				return ranges_ascii();
			}
			else
			{
				return ranges_all();
			}
		}();

		const auto g = [&random, &ranges](auto& dest, [[maybe_unused]] const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			std::size_t max_try = 3;
			while (max_try--)
			{
				auto v = random.get<std::uint32_t>(from, to);

				if (v < 0x80)
				{
					const auto v1 = static_cast<char_type>(v);
					if constexpr (Little)
					{
						dest.push_back(v1);
					}
					else
					{
						const auto v1_swap = std::byteswap(v1);
						dest.push_back(v1_swap);
					}

					return 1;
				}

				if constexpr (not AsciiOnly)
				{
					if (v >= 0xd800 and v <= 0xdfff)
					{
						// skip surrogate pairs
						v += (0xdfff - 0xd800 + 1);
					}

					if (v < 0xffff)
					{
						const auto v1 = static_cast<char_type>(v);
						if constexpr (Little)
						{
							dest.push_back(v1);
						}
						else
						{
							const auto v1_swap = std::byteswap(v1);
							dest.push_back(v1_swap);
						}

						return 1;
					}

					if (remaining >= 2)
					{
						v -= 0x1'0000;
						const auto high_surrogate = static_cast<char_type>(0xd800 | ((v >> 10) & 0x3ff));
						const auto low_surrogate = static_cast<char_type>(0xdc00 | (v & 0x3ff));

						if constexpr (Little)
						{
							dest.push_back(high_surrogate);
							dest.push_back(low_surrogate);
						}
						else
						{
							const auto high_surrogate_swap = std::byteswap(high_surrogate);
							const auto low_surrogate_swap = std::byteswap(low_surrogate);
							dest.push_back(high_surrogate_swap);
							dest.push_back(low_surrogate_swap);
						}

						return 2;
					}
				}
			}

			dest.push_back(u'?');
			return 1;
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::generate_string<char_type>(g, length);
	}

	// =====================================================
	// UTF32

	template<bool AsciiOnly = false>
	[[nodiscard]] auto make_random_utf32_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
	{
		using namespace gal::prometheus::numeric;

		using char_type = char32_t;

		Random<RandomStateCategory::PRIVATE, random_engine_xrsr_128_plus_plus> random{};
		const auto& ranges = []() noexcept
		{
			if constexpr (AsciiOnly)
			{
				return ranges_ascii();
			}
			else
			{
				return ranges_all();
			}
		}();

		const auto g = [&random, &ranges](auto& dest, [[maybe_unused]] const auto remaining) noexcept -> std::size_t
		{
			const auto& [from, to] = ranges[random.get<std::size_t>(0, ranges.size() - 1)];

			auto v = random.get<std::uint32_t>(from, to);

			if (v < 0x80)
			{
				const auto v1 = static_cast<char_type>(v);
				dest.push_back(v1);
				return 1;
			}

			if constexpr (not AsciiOnly)
			{
				if (v >= 0xd800 and v <= 0xdfff)
				{
					// skip surrogate pairs
					v += (0xdfff - 0xd800 + 1);
				}

				const auto v1 = static_cast<char_type>(v);
				dest.push_back(v1);

				return 1;
			}
			else
			{
				GAL_PROMETHEUS_ERROR_UNREACHABLE();
			}
		};

		const auto length = random.get(min_length, max_length);
		return gen_detail::generate_string<char_type>(g, length);
	}
}

// ==============================
// LATIN

[[nodiscard]] inline auto make_random_latin_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_latin_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_latin_string<char, true>(min_length, max_length);
}

// ==============================
// UTF8

[[nodiscard]] inline auto make_random_utf8_char_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_utf8_string<char>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_char_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char>
{
	return gen_detail::make_random_utf8_string<char, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char8_t>
{
	return gen_detail::make_random_utf8_string<char8_t>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf8_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char8_t>
{
	return gen_detail::make_random_utf8_string<char8_t, true>(min_length, max_length);
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
	return gen_detail::make_random_utf16_string<true, true>(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf16_be_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char16_t>
{
	return gen_detail::make_random_utf16_string<false, true>(min_length, max_length);
}

// ==============================
// UTF32

[[nodiscard]] inline auto make_random_utf32_string(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
{
	return gen_detail::make_random_utf32_string(min_length, max_length);
}

[[nodiscard]] inline auto make_random_utf32_string_ascii_only(const std::size_t min_length = 0, const std::size_t max_length = 65535) noexcept -> std::basic_string<char32_t>
{
	return gen_detail::make_random_utf32_string<true>(min_length, max_length);
}
