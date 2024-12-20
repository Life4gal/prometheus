#include <unit_test/unit_test.hpp>
#include <chars/icelake.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	// todo: Simd<"icelake.utf8_char"> / Simd<"icelake.utf8"> / Simd<"utf32">
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.latin.icelake"> _ = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string_ascii_only();

				expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::LATIN>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::LATIN>(source, dest.data());
					expect(error.has_error() != "valid latin string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::LATIN>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string();

				// expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::UTF8_CHAR>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::UTF8_CHAR>(source, dest.data());
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF8_CHAR>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string();

				// expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::UTF8>(source);

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::UTF8>(source, dest.data());
					expect(error.has_error() != "valid utf8 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF8>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string();

				// expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::UTF16>(source);

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::UTF16_LE>(source, dest.data());
					expect(error.has_error() != "valid utf16_le string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
					expect(valid == "valid utf16_le string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF16_LE>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
					expect(valid == "valid utf16_le string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string();

				// expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::UTF16>(source);

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::UTF16_BE>(source, dest.data());
					expect(error.has_error() != "valid utf16_be string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
					expect(valid == "valid utf16_be string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF16_BE>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
					expect(valid == "valid utf16_be string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_latin_string();

				// expect(Simd<"icelake.latin">::validate<true>(source) == "valid latin string"_b) << fatal;
				const auto output_length = Simd<"icelake.latin">::length<CharsType::UTF32>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.latin">::convert<CharsType::UTF32>(source, dest.data());
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF32>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.latin">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.latin">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};
	};
}
