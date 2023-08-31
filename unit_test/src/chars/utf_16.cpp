#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;
import gal.prometheus.chars;

GAL_PROMETHEUS_DISABLE_WARNING_PUSH
GAL_PROMETHEUS_DISABLE_WARNING_MSVC(4819)

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace chars;

	auto generate_code_point() noexcept -> char32_t
	{
		static std::mt19937                  rand{std::random_device{}()};
		static std::uniform_int_distribution size_dist{0, 99};
		static std::uniform_int_distribution ascii_dist{0, 0x7f};
		static std::uniform_int_distribution latin_dist{0x80, 0x7ff};
		static std::uniform_int_distribution basic_dist{0x800, 0xf7ff};// without surrogates
		static std::uniform_int_distribution full_dist{0x010'000, 0x10f'fff};

		const auto s = size_dist(rand);
		if (s < 90) { return infrastructure::char_cast<char32_t>(ascii_dist(rand)); }

		if (s < 95) { return infrastructure::char_cast<char32_t>(latin_dist(rand)); }

		if (s < 98)
		{
			auto c = infrastructure::char_cast<char32_t>(basic_dist(rand));
			if (c >= 0xd800 and c < 0xe000) { c += 0x800; }
			return c;
		}

		return infrastructure::char_cast<char32_t>(full_dist(rand));
	}

	auto generate_string(const char32_t code_point, std::u16string& string) noexcept -> void
	{
		if (code_point < 0x01'0000) { string += infrastructure::char_cast<char16_t>(code_point); }
		else
		{
			const auto cp = code_point - 0x01'0000;
			string += infrastructure::char_cast<char16_t>(0xd800 + (cp >> 10));
			string += infrastructure::char_cast<char16_t>(0xdc00 + (cp & 0x03ff));
		}
	}

	auto is_valid_split(const std::u16string_view string) noexcept -> bool
	{
		if (string.size() == 0) { return true; }

		if (string.size() == 1) { return string.front() < 0xd800 or string.front() >= 0xe000; }

		if (string.front() >= 0xdc00 and string.front() < 0xe000) { return false; }

		if (string.back() >= 0xd800 and string.back() < 0xdc00) { return false; }

		return true;
	}

	GAL_PROMETHEUS_NO_DESTROY suite test_chars_utf_16 = []
	{
		"copy_check"_test = []
		{
			std::u16string identity{};
			identity.reserve(100);
			for (int i = 0; i < 100; ++i) { generate_string(generate_code_point(), identity); }

			for (std::u16string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::u16string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);

					if (not is_valid_split(origin)) { continue; }

					const auto result = CharConverter<char_map_category_utf_16, char_map_category_utf_16>{}.operator()<std::u16string>(origin);

					expect((origin == result) >> fatal);
				}
			}
		};

		"move_check"_test = []
		{
			std::u16string identity{};
			identity.reserve(100);
			for (int i = 0; i < 100; ++i) { generate_string(generate_code_point(), identity); }

			for (std::u16string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::u16string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);

					if (not is_valid_split(origin)) { continue; }

					auto       copy   = origin;
					const auto result = CharConverter<char_map_category_utf_16, char_map_category_utf_16>{}.operator()<std::u16string>(std::move(copy));

					expect((copy.empty() == "moved"_b) >> fatal);
					expect((origin == result) >> fatal);
				}
			}
		};

		"invalid_char_conversion"_test = []
		{
			constexpr std::u16string_view text_with_invalid_chars{
					u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "
					u"\xd800 abc "          // 
					u"\xdc00 abc "          // 
					u"\U00012345 abc "      // 
					u"\xd800\U00012345 abc "// 
					u"\xdc00\U00012345 abc "//
			};
			constexpr std::u16string_view text_after_conversion{
					u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "
					u"\ufffd abc "          // 
					u"\ufffd abc "          // 
					u"\U00012345 abc "      // 
					u"\ufffd\U00012345 abc "// 
					u"\ufffd\U00012345 abc "//
			};

			for (std::string::size_type i = 0; i < text_with_invalid_chars.size(); ++i)
			{
				for (std::string::size_type j = i; j < text_with_invalid_chars.size(); ++j)
				{
					const auto origin = text_with_invalid_chars.substr(i, j - i);

					if (not is_valid_split(origin)) { continue; }

					const auto expected = text_after_conversion.substr(i, j - i);

					const auto result = CharConverter<char_map_category_utf_16, char_map_category_utf_16>{}.operator()<std::u16string>(origin);

					expect((expected == result) >> fatal);
				}
			}
		};
	};
}// namespace

GAL_PROMETHEUS_DISABLE_WARNING_POP
