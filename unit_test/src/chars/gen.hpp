#pragma once

// numeric::random
#include <numeric/numeric.hpp>
// i18n::range
#include <i18n/i18n.hpp>
// unit_test
#include <unit_test/unit_test.hpp>

using namespace gal::prometheus; // NOLINT(clang-diagnostic-header-hygiene)

namespace gen_detail
{
	// =====================================================

	[[nodiscard]] inline auto ranges_ascii() noexcept -> const auto&
	{
		const static auto range = i18n::RangeBuilder{}.ascii().range();

		return range;
	}

	[[nodiscard]] inline auto ranges_latin() noexcept -> const auto&
	{
		const static auto range = i18n::RangeBuilder{}.latin().range();

		return range;
	}

	[[nodiscard]] inline auto ranges_all() noexcept -> const auto&
	{
		const static auto range = i18n::RangeBuilder{}.latin().greek().korean().japanese().simplified_chinese_common().range();

		return range;
	}

	template<typename Char>
	[[nodiscard]] auto generate_string(const auto& generator, const std::size_t length) noexcept -> std::basic_string<Char>
	{
		std::basic_string<Char> result{};
		result.reserve(length);

		auto remaining = length - 1;
		while (remaining -= generator(result, remaining)) {}

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

template<
	typename From,
	typename To,
	// Xxx<"latin">::validate(source) ==> PURE ASCII ONLY
	bool ValidateSource = true,
	typename Source
>
constexpr auto make_test(const Source& source) noexcept -> void
{
	using namespace gal::prometheus::unit_test;

	using out_char_type = typename To::char_type;
	using out_type = std::basic_string<out_char_type>;
	constexpr auto out_chars_type = To::chars_type;

	if constexpr (ValidateSource)
	{
		expect(From::template validate<true>(source) == "valid source string"_b) << fatal;
	}

	const auto source_length = From::template length<From::chars_type>(source);
	const auto output_length = From::template length<out_chars_type>(source);

	{
		out_type dest{};
		dest.resize(output_length);

		const auto convert_result = From::template convert<out_chars_type, chars::InputProcessPolicy::DEFAULT>(source, dest.data());
		expect(convert_result.has_error() != "valid source string"_b) << fatal;
		expect(convert_result.input == value(source_length)) << fatal;

		const auto validate_output_result = To::template validate<true>(dest);
		expect(validate_output_result == "valid output string"_b) << fatal;

		const auto result = From::template convert<out_type, out_chars_type, chars::InputProcessPolicy::DEFAULT>(source);
		expect(dest == ref(result)) << fatal;
	}

	if constexpr (ValidateSource)
	{
		out_type dest{};
		dest.resize(output_length);

		const auto convert_output_length = From::template convert<out_chars_type, chars::InputProcessPolicy::ASSUME_ALL_CORRECT>(source, dest.data());
		expect(convert_output_length == value(dest.size())) << fatal;

		const auto validate_output_result = To::template validate<true>(dest);
		expect(validate_output_result == "valid output string"_b) << fatal;

		const auto result = From::template convert<out_type, out_chars_type, chars::InputProcessPolicy::ASSUME_ALL_CORRECT>(source);
		expect(dest == ref(result)) << fatal;
	}
}

template<
	typename From,
	typename To,
	// Xxx<"latin">::validate(source) ==> PURE ASCII ONLY
	bool ValidateSourceOnly = false,
	typename Source
>
constexpr auto make_test(
	const Source& source,
	const chars::ErrorCode expected_error,
	const std::size_t expected_in
) noexcept -> void
{
	using namespace gal::prometheus::unit_test;

	using out_char_type = typename To::char_type;
	using out_type = std::basic_string<out_char_type>;
	constexpr auto out_chars_type = To::chars_type;

	const auto output_length = From::template length<out_chars_type>(source);

	const auto validate_source_result = From::template validate<true>(source);
	expect(validate_source_result.has_error() == "invalid source string"_b) << fatal;
	expect(validate_source_result.error == value(expected_error)) << fatal;
	expect(validate_source_result.input == value(expected_in)) << fatal;

	if constexpr (not ValidateSourceOnly)
	{
		out_type dest{};
		dest.resize(output_length);

		const auto convert_result = From::template convert<out_chars_type, chars::InputProcessPolicy::DEFAULT>(source, dest.data());
		expect(convert_result.has_error() == "invalid source string"_b) << fatal;
		expect(convert_result.error == value(expected_error)) << fatal;
		expect(convert_result.input == value(expected_in)) << fatal;

		const auto validate_output_result = To::template validate<true>(dest);
		expect(validate_output_result == "valid output string"_b) << fatal;

		const auto result = From::template convert<out_chars_type, chars::InputProcessPolicy::DEFAULT>(source);
		expect(dest == ref(result)) << fatal;
	}
}

// ==============================
// LATIN

template<typename From>
constexpr auto make_test_latin_error() noexcept -> void
{
	using namespace gal::prometheus::unit_test;

	"too_large"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0x80, // 10 <-- ERROR
				0x61, // 11
				0x61, // 12
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char*>(source_u8), source_length};

		make_test<From, From, true>(source, chars::ErrorCode::TOO_LARGE, 10);
	};
}

// ==============================
// UTF8/UTF8_CHAR

template<typename From>
constexpr auto make_test_utf8_error() noexcept -> void
{
	using namespace gal::prometheus::unit_test;

	using char_type = typename From::char_type;

	"overlong"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xC0, 0xAF, // 10+11 <-- ERROR '/'(U+002F)
				0x61, // 12
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::OVERLONG, 10);
	};

	"surrogate"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xED, 0xA0, 0x80, // 10+11+12 <-- ERROR (U+D800)
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::SURROGATE, 10);
	};

	"bad continuation byte"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xC2, // 10 <-- ERROR (missing/invalid) continuation byte
				0x61, // 11
				0x61, // 12
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::TOO_SHORT, 10);
	};

	"too many continuation bytes"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0x80, 0x80, // 10+11 <-- ERROR too many continuation bytes/missing leading byte
				0x61, // 12
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::TOO_LONG, 10);
	};

	"header bits"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xF8, 0x88, 0x80, 0x80, 0x80, // 10+11+12+13+14 <-- ERROR invalid leading byte
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::HEADER_BITS, 10);
	};

	"too large"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xF4, 0x90, 0x80, 0x80, // 10+11+12+13 <-- ERROR (U+110000) greater than U+10FFFF
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::TOO_LARGE, 10);
	};

	"truncated"_test = []
	{
		constexpr std::uint8_t source_u8[]
		{
				0x61, // 0
				0x61, // 1
				0x61, // 2
				0x61, // 3 
				0x61, // 4
				0x61, // 5
				0x61, // 6
				0x61, // 7
				0x61, // 8
				0x61, // 9
				0xF0, 0x9F, 0x98, // 10+11+13 <-- ERROR emoji(😀), missing last byte(0x80)
				0x61, // 13
				0x61, // 14
				0x61, // 15
				0x61, // 16
				0x61, // 17
				0x61, // 18
				0x61, // 19
				0x61, // 20
				0x61, // 21
				0x61, // 22
				0x61, // 23
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61,
				0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8);
		const auto source = std::span{reinterpret_cast<const char_type*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::TOO_SHORT, 10);
	};
}

// ==============================
// UTF16_LE/UTF16_BE

template<typename From>
constexpr auto make_test_utf16_error() noexcept -> void
{
	using namespace gal::prometheus::unit_test;

	// high + ?
	"missing/invalid low surrogate"_test = []
	{
		alignas(alignof(char16_t)) constexpr std::uint8_t source_u8[]
		{
				0x61, 0x00, // 0
				0x61, 0x00, // 1
				0x61, 0x00, // 2
				0x61, 0x00, // 3
				0x61, 0x00, // 4
				0x61, 0x00, // 5
				0x61, 0x00, // 6
				0x61, 0x00, // 7
				0x61, 0x00, // 8
				0x61, 0x00, // 9
				0x00, 0xD8, // 10 <-- ERROR high surrogate only, missing/invalid low surrogate 
				0x61, 0x00, // 11
				0x61, 0x00, // 12
				0x61, 0x00, // 13
				0x61, 0x00, // 14
				0x61, 0x00, // 15
				0x61, 0x00, // 16
				0x61, 0x00, // 17
				0x61, 0x00, // 18
				0x61, 0x00, // 19
				0x61, 0x00, // 20
				0x61, 0x00, // 21
				0x61, 0x00, // 22
				0x61, 0x00, // 23
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x00, 0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8) / 2;
		const auto source = std::span{reinterpret_cast<const char16_t*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::SURROGATE, 10);
	};

	// ? + low
	"unexpected low surrogate"_test = []
	{
		alignas(alignof(char16_t)) constexpr std::uint8_t source_u8[]
		{
				0x61, 0x00, // 0
				0x61, 0x00, // 1
				0x61, 0x00, // 2
				0x61, 0x00, // 3
				0x61, 0x00, // 4
				0x61, 0x00, // 5
				0x61, 0x00, // 6
				0x61, 0x00, // 7
				0x61, 0x00, // 8
				0x61, 0x00, // 9
				0x00, 0xDC, // 10 <-- ERROR unexpected low surrogate
				0x61, 0x00, // 11
				0x61, 0x00, // 12
				0x61, 0x00, // 13
				0x61, 0x00, // 14
				0x61, 0x00, // 15
				0x61, 0x00, // 16
				0x61, 0x00, // 17
				0x61, 0x00, // 18
				0x61, 0x00, // 19
				0x61, 0x00, // 20
				0x61, 0x00, // 21
				0x61, 0x00, // 22
				0x61, 0x00, // 23
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x00, 0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8) / 2;
		const auto source = std::span{reinterpret_cast<const char16_t*>(source_u8), source_length};

		make_test<From, From>(source, chars::ErrorCode::SURROGATE, 10);
	};

	// high + high
	"duo high surrogate"_test = []
	{
		alignas(alignof(char16_t)) constexpr std::uint8_t source_u8[]
		{
				0x61, 0x00, // 0
				0x61, 0x00, // 1
				0x61, 0x00, // 2
				0x61, 0x00, // 3
				0x61, 0x00, // 4
				0x61, 0x00, // 5
				0x61, 0x00, // 6
				0x61, 0x00, // 7
				0x61, 0x00, // 8
				0x61, 0x00, // 9
				0x00, 0xD8, 0x01, 0xD8, // 10+11 <-- ERROR duo high surrogate
				0x61, 0x00, // 12
				0x61, 0x00, // 13
				0x61, 0x00, // 14
				0x61, 0x00, // 15
				0x61, 0x00, // 16
				0x61, 0x00, // 17
				0x61, 0x00, // 18
				0x61, 0x00, // 19
				0x61, 0x00, // 20
				0x61, 0x00, // 21
				0x61, 0x00, // 22
				0x61, 0x00, // 23
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x00, 0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8) / 2;
		const auto source = std::span{reinterpret_cast<const char16_t*>(source_u8), source_length};

		make_test<From, From, true>(source, chars::ErrorCode::SURROGATE, 10);
	};

	// low + low
	"duo low surrogate"_test = []
	{
		alignas(alignof(char16_t)) constexpr std::uint8_t source_u8[]
		{
				0x61, 0x00, // 0
				0x61, 0x00, // 1
				0x61, 0x00, // 2
				0x61, 0x00, // 3
				0x61, 0x00, // 4
				0x61, 0x00, // 5
				0x61, 0x00, // 6
				0x61, 0x00, // 7
				0x61, 0x00, // 8
				0x61, 0x00, // 9
				0x00, 0xDC, 0x01, 0xDC, // 10+11 <-- ERROR duo low surrogate
				0x61, 0x00, // 12
				0x61, 0x00, // 13
				0x61, 0x00, // 14
				0x61, 0x00, // 15
				0x61, 0x00, // 16
				0x61, 0x00, // 17
				0x61, 0x00, // 18
				0x61, 0x00, // 19
				0x61, 0x00, // 20
				0x61, 0x00, // 21
				0x61, 0x00, // 22
				0x61, 0x00, // 23
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00,
				0x00, 0x00,
		};

		constexpr auto source_length = std::ranges::size(source_u8) / 2;
		const auto source = std::span{reinterpret_cast<const char16_t*>(source_u8), source_length};

		make_test<From, From, true>(source, chars::ErrorCode::SURROGATE, 10);
	};
}

// ==============================
// UTF32

template<typename From>
constexpr auto make_test_utf32_error() noexcept -> void
{
	using namespace gal::prometheus::unit_test;
	// todo
}
