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

	[[nodiscard]] auto make_source(auto generator, const std::size_t size) -> std::basic_string<char32_t>
	{
		std::basic_string<char32_t> source{};
		for (std::size_t i = 0; i < size;)
		{
			i += generator(source);
		}

		return source;
	}

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf32"> _ = []
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

			source.push_back(static_cast<char32_t>(v));
			return 1;
		};
		const auto generator_char_only = [&random](auto& source) mutable -> std::size_t
		{
			source.push_back(static_cast<char>(random.get<unsigned>() & std::numeric_limits<char>::max()));
			return 1;
		};

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_ascii"_test = [source = make_source(generator_char_only, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::ASCII>(source);
				expect(chars::validate<chars::CharsCategory::ASCII>(dest) == "valid ascii string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::ASCII, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::ASCII>(dest_a) == "valid ascii string"_b) << fatal;
			};

			"to_utf8_char"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF8_CHAR>(source);
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(dest) == "valid utf8 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF8_CHAR, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF8_CHAR>(dest_a) == "valid utf8 string"_b) << fatal;
			};

			"to_utf8"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF8>(source);
				expect(chars::validate<chars::CharsCategory::UTF8>(dest) == "valid utf8 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF8, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF8>(dest_a) == "valid utf8 string"_b) << fatal;
			};

			"to_utf16_le"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF16_LE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest) == "valid utf16_le string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF16_LE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(dest_a) == "valid utf16_le string"_b) << fatal;
			};

			"to_utf16_be"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF16_BE>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest) == "valid utf16_be string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF16_BE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(dest_a) == "valid utf16_be string"_b) << fatal;
			};

			"to_utf32"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::UTF32>(source) == "valid utf32 string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::UTF32, chars::CharsCategory::UTF32>(source) == value(source.size())) << fatal;

				const auto dest = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF32>(source);
				expect((dest == ref(source)) == "valid utf32 string"_b) << fatal;

				const auto dest_a = chars::convert<chars::CharsCategory::UTF32, chars::CharsCategory::UTF32, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect((dest_a == ref(source)) == "valid utf32 string"_b) << fatal;
			};
		}

		config().output_level = old_level;
	};
}
