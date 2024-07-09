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

	[[nodiscard]] auto make_source(auto generator, const auto size) -> std::basic_string<char>
	{
		std::basic_string<char> source{};
		source.resize(size);
		std::ranges::generate_n(source.begin(), size, generator);

		return source;
	}

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.ascii"> _ = []
	{
		using namespace unit_test;
		using namespace literals;

		const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		numeric::Random<numeric::RandomStateCategory::PRIVATE, numeric::random_engine_xrsr_128_plus_plus> random;
		const auto generator = [&random]() mutable
		{
			return static_cast<char>(random.get<unsigned>() & std::numeric_limits<char>::max());
		};

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_ascii"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::ASCII>(source) == value{source.size()}) << fatal;
				expect((chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::ASCII>(source) == source) == "valid ascii string"_b) << fatal;
				expect((chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::ASCII, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source) == source) == "valid ascii string"_b) << fatal;
			};

			"to_utf8_char"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8_CHAR>(source) == value{source.size()}) << fatal;
				expect((chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8_CHAR>(source) == source) == "valid utf8_char string"_b) << fatal;
				expect((chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8_CHAR, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source) == source) == "valid utf8_char string"_b) << fatal;
			};

			"to_utf8"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8>(source) == value{source.size()}) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF8>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8>(source)) == "valid utf8 string"_b) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF8>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF8, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf8 string"_b) << fatal;
			};

			"to_utf16_le"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_LE>(source) == value{source.size()}) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_LE>(source)) == "valid utf16_le string"_b) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF16_LE>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_LE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf16_le string"_b) << fatal;
			};

			"to_utf16_be"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_BE>(source) == value{source.size()}) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_BE>(source)) == "valid utf16_be string"_b) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF16_BE>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF16_BE, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf16_be string"_b) << fatal;
			};

			"to_utf32"_test = [source = make_source(generator, random.get<std::size_t>(0, 65535))]
			{
				using operators::operator==;
				expect(chars::validate<chars::CharsCategory::ASCII>(source) == "valid ascii string"_b) << fatal;
				expect(chars::length<chars::CharsCategory::ASCII, chars::CharsCategory::UTF32>(source) == value{source.size()}) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF32>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF32>(source)) == "valid utf32 string"_b) << fatal;
				expect(chars::validate<chars::CharsCategory::UTF32>(chars::convert<chars::CharsCategory::ASCII, chars::CharsCategory::UTF32, chars::InputProcessPolicy::ASSUME_VALID_INPUT>(source)) == "valid utf32 string"_b) << fatal;
			};
		}

		config().output_level = old_level;
	};
}
