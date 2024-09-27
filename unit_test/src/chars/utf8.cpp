#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_USE_MODULE
import std;
import gal.prometheus;
#else
#include <prometheus.ixx>

#include <algorithm>
#include <string>
#include <limits>
#endif

namespace
{
	using namespace gal::prometheus;

	template<typename Char>
	[[nodiscard]] auto make_source(auto generator, const std::size_t size) -> std::basic_string<Char>
	{
		std::basic_string<Char> source{};
		for (std::size_t i = 0; i < size;)
		{
			i += generator(source);
		}

		return source;
	}

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8_char"> chars_utf8_char = []
	{
		using namespace unit_test;

		const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		numeric::Random<numeric::RandomStateCategory::PRIVATE, numeric::random_engine_xrsr_128_plus_plus> random;
		const auto generator = [&random](auto& source) mutable -> std::size_t
		{
			unsigned v;
			do
			{
				v = random.get<unsigned>(0, 0x0010'ffef);
			} while (
				// skip surrogate pairs
				(v >= 0xd800 and v <= 0xdfff) // or
				// // skip non-characters
				// (v >= 0xfdd0 and v <= 0xfdef) or (v & 0xfffe) == 0xfffe or
				// // skip control characters
				// (v <= 0x001f) or (v >= 0x007f and v <= 0x009f) or
				// // skip special characters
				// (v >= 0xfff0 and v <= 0xfff8) or
				// // skip tag characters and variation selector supplement
				// (v >= 0x000e'0000 and v <= 0x000e'01ef) or
				// // skip private use areas
				// (v >= 0x000f'0000)
			);

			if (v < 0x0080)
			{
				source.push_back(static_cast<char>(v));
				return 1;
			}
			else if (v < 0x0800)
			{
				source.push_back(static_cast<char>(0xc0 | (v >> 6)));
				source.push_back(static_cast<char>(0x80 | (v & 0x3f)));
				return 2;
			}
			else if (v < 0x0001'0000)
			{
				source.push_back(static_cast<char>(0xe0 | (v >> 12)));
				source.push_back(static_cast<char>(0x80 | ((v >> 6) & 0x3f)));
				source.push_back(static_cast<char>(0x80 | (v & 0x3f)));
				return 3;
			}
			else
			{
				source.push_back(static_cast<char>(0xf0 | (v >> 18)));
				source.push_back(static_cast<char>(0x80 | ((v >> 12) & 0x3f)));
				source.push_back(static_cast<char>(0x80 | ((v >> 6) & 0x3f)));
				source.push_back(static_cast<char>(0x80 | (v & 0x3f)));
				return 4;
			}
		};
		const auto generator_char_only = [&random](auto& source) mutable -> std::size_t
		{
			source.push_back(static_cast<char>(random.get<unsigned>() & std::numeric_limits<char>::max()));
			return 1;
		};

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_ascii"_test = [source = make_source<char>(generator_char_only, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII>(source);
				expect((dest == ref(source)) == "valid ascii string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect((dest_a == ref(source)) == "valid ascii string"_b) << fatal;
			};

			"to_utf8_char"_test = [source = make_source<char>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR>(source);
				expect((dest == ref(source)) == "valid utf8_char string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect((dest_a == ref(source)) == "valid utf8_char string"_b) << fatal;
			};

			"to_utf8"_test = [source = make_source<char>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8>(source);
				expect(chars::validate<chars::CharsCategory::UTF8>(dest) == "valid utf8 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF8>(dest_a) == "valid utf8 string"_b) << fatal;
			};

			"to_utf16_le"_test = [source = make_source<char>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_LE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest) == "valid utf16_le string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_LE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest_a) == "valid utf16_le string"_b) << fatal;
			};

			"to_utf16_be"_test = [source = make_source<char>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_BE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest) == "valid utf16_be string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_BE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest_a) == "valid utf16_be string"_b) << fatal;
			};

			"to_utf32"_test = [source = make_source<char>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(source);
				expect(chars::validate<chars::CharsCategory::UTF32>(dest) == "valid utf32 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF32>(dest_a) == "valid utf32 string"_b) << fatal;
			};
		}

		config().output_level = old_level;
	};

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8"> chars_utf8 = []
	{
		using namespace unit_test;

		const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		numeric::Random<numeric::RandomStateCategory::PRIVATE, numeric::random_engine_xrsr_128_plus_plus> random;
		const auto generator = [&random](auto& source) mutable -> std::size_t
		{
			unsigned v;
			do
			{
				v = random.get<unsigned>(0, 0x0010'ffef);
			} while (
				// skip surrogate pairs
				(v >= 0xd800 and v <= 0xdfff) // or
				// // skip non-characters
				// (v >= 0xfdd0 and v <= 0xfdef) or (v & 0xfffe) == 0xfffe or
				// // skip control characters
				// (v <= 0x001f) or (v >= 0x007f and v <= 0x009f) or
				// // skip special characters
				// (v >= 0xfff0 and v <= 0xfff8) or
				// // skip tag characters and variation selector supplement
				// (v >= 0x000e'0000 and v <= 0x000e'01ef) or
				// // skip private use areas
				// (v >= 0x000f'0000) or (v >= 0xe000 and v <= 0xf8ff) or
				// // skip ideographic description characters
				// (v >= 0x2ff0 and v <= 0x2fff)
			);

			if (v < 0x0080)
			{
				source.push_back(static_cast<char8_t>(v));
				return 1;
			}
			else if (v < 0x0800)
			{
				source.push_back(static_cast<char8_t>(0xc0 | (v >> 6)));
				source.push_back(static_cast<char8_t>(0x80 | (v & 0x3f)));
				return 2;
			}
			else if (v < 0x0001'0000)
			{
				source.push_back(static_cast<char8_t>(0xe0 | (v >> 12)));
				source.push_back(static_cast<char8_t>(0x80 | ((v >> 6) & 0x3f)));
				source.push_back(static_cast<char8_t>(0x80 | (v & 0x3f)));
				return 3;
			}
			else
			{
				source.push_back(static_cast<char8_t>(0xf0 | (v >> 18)));
				source.push_back(static_cast<char8_t>(0x80 | ((v >> 12) & 0x3f)));
				source.push_back(static_cast<char8_t>(0x80 | ((v >> 6) & 0x3f)));
				source.push_back(static_cast<char8_t>(0x80 | (v & 0x3f)));
				return 4;
			}
		};
		const auto generator_char_only = [&random](auto& source) mutable -> std::size_t
		{
			source.push_back(static_cast<char8_t>(random.get<unsigned>() & std::numeric_limits<char>::max()));
			return 1;
		};

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_ascii"_test = [source = make_source<char8_t>(generator_char_only, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8, chars::CharsCategory::ASCII>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::ASCII>(source);
				expect(chars::validate<chars::CharsCategory::ASCII>(dest) == "valid ascii string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::ASCII, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::ASCII>(dest_a) == "valid ascii string"_b) << fatal;
			};

			"to_utf8_char"_test = [source = make_source<char8_t>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8_CHAR>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8_CHAR>(source);
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(dest) == "valid utf8_char string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8_CHAR, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(dest_a) == "valid utf8_char string"_b) << fatal;
			};

			"to_utf8"_test = [source = make_source<char8_t>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8>(source);
				expect((dest == ref(source)) == "valid utf8 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF8, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect((dest_a == ref(source)) == "valid utf8 string"_b) << fatal;
			};

			"to_utf16_le"_test = [source = make_source<char8_t>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF16_LE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest) == "valid utf16_le string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF16_LE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest_a) == "valid utf16_le string"_b) << fatal;
			};

			"to_utf16_be"_test = [source = make_source<char8_t>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF16_BE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest) == "valid utf16_be string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF16_BE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest_a) == "valid utf16_be string"_b) << fatal;
			};

			"to_utf32"_test = [source = make_source<char8_t>(generator, random.get<std::size_t>(0, 65535))]
			{
				expect(chars::validate<chars::CharsCategory::UTF8>(source) == "valid utf8 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF32>(source);
				expect(chars::validate<chars::CharsCategory::UTF32>(dest) == "valid utf32 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF8, chars::CharsCategory::UTF32, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF32>(dest_a) == "valid utf32 string"_b) << fatal;
			};
		}

		config().output_level = old_level;
	};
}
