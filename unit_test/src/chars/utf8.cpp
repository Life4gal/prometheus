#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_USE_MODULE
import std;

import gal.prometheus.infrastructure;
import gal.prometheus.chars;
import gal.prometheus.numeric;
#else
#include <algorithm>
#include <string>
#include <limits>

#include <infrastructure/infrastructure.ixx>
#include <chars/chars.ixx>
#include <numeric/numeric.ixx>
#endif

namespace 
{
	using namespace gal::prometheus;

	template<typename Char>
	[[nodiscard]] auto make_source(auto generator, const std::size_t size) -> std::basic_string<Char>
	{
		std::basic_string<Char> source{};
		for (std::size_t i = 0; i < size; )
		{
			i += generator(source);
		}

		return source;
	}

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8_char"> _ = []
	{
		using namespace unit_test;
		using namespace literals;

		const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		numeric::Random<numeric::RandomStateCategory::PRIVATE, numeric::random_engine_xrsr_128_plus_plus> random;
		const auto generator = [&random](auto& source) mutable -> std::size_t
		{
			unsigned v;
			do
			{
				v = random.get<unsigned>(0, 0x0010'ffef);
			}
			// skip surrogate pairs
			while (v >= 0xd800 && v <= 0xdfff); 

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
			"to_ascii"_test = [source = make_source<char>(generator_char_only, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII>(source) == value{source.size()}) << fatal;
				expect((chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII>(source) == source) == "valid ascii string"_b) << fatal;
				expect((chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::ASCII, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source) == source) == "valid ascii string"_b) << fatal;
			};

			"to_utf8_char"_test = [source = make_source<char>(generator, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b)<< fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR>(source) == value{source.size()}) << fatal;
				expect((chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR>(source) == source) == "valid utf8_char string"_b) << fatal;
				expect((chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8_CHAR, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source) == source) == "valid utf8_char string"_b) << fatal;
			};

			"to_utf8"_test = [source = make_source<char>(generator, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8>(source) == value{source.size()}) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF8>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8>(source)) == "valid utf8 string"_b) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF8>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF8, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf8 string"_b) << fatal;
			};
			
			// "to_utf16_le"_test = [source = make_source<char>(generator, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			// {
			// 	using operators::operator==;
			// 	expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
			// 	expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_LE>(source) == value{source.size()}) << fatal;
			// 	expect(chars::validate<chars::CharsCategory::UTF16_LE>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_LE>(source)) == "valid utf16_le string"_b) << fatal;
			// 	// expect(chars::validate<chars::CharsCategory::UTF16_LE>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_LE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf16_le string"_b) << fatal;
			// };
			//
			// "to_utf16_be"_test = [source = make_source<char>(generator, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			// {
			// 	using operators::operator==;
			// 	expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
			// 	expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_BE>(source) == value{source.size()}) << fatal;
			// 	expect(chars::validate<chars::CharsCategory::UTF16_BE>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_BE>(source)) == "valid utf16_be string"_b) << fatal;
			// 	// expect(chars::validate<chars::CharsCategory::UTF16_BE>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF16_BE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf16_be string"_b) << fatal;
			// };
			//
			// "to_utf32"_test = [source = make_source<char>(generator, random.get<std::size_t>(std::numeric_limits<char>::max() / 2, std::numeric_limits<char>::max()))]
			// {
			// 	using operators::operator==;
			// 	expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(source) == "valid utf8_char string"_b) << fatal;
			// 	expect(chars::length<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(source) == value{source.size()}) << fatal;
			// 	expect(chars::validate<chars::CharsCategory::UTF32>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32>(source)) == "valid utf32 string"_b) << fatal;
			// 	// expect(chars::validate<chars::CharsCategory::UTF32>(chars::convert<chars::CharsCategory::UTF8_CHAR, chars::CharsCategory::UTF32, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf32 string"_b) << fatal;
			// };
		}

		config().output_level = old_level;
	};
}
