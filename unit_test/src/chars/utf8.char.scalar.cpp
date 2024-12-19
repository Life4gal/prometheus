#include <unit_test/unit_test.hpp>
#include <chars/scalar.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8.char.scalar"> _ = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string_ascii_only();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;
				expect(Scalar<"utf8.char">::length<CharsType::LATIN>(source) == value(source.size())) << fatal;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::LATIN>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::LATIN>(source, dest.data());

					expect(error.has_error() != "valid latin string"_b) << fatal;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::LATIN>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::LATIN>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;
				expect(Scalar<"utf8.char">::length<CharsType::UTF8_CHAR>(source) == value(source.size())) << fatal;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF8_CHAR>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::UTF8_CHAR>(source, dest.data());

					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF8_CHAR>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF8_CHAR>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;
				expect(Scalar<"utf8.char">::length<CharsType::UTF8>(source) == value(source.size())) << fatal;

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF8>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::UTF8>(source, dest.data());

					expect(error.has_error() != "valid utf8 string"_b) << fatal;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF8>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF8>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF16_LE>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::UTF16_LE>(source, dest.data());

					expect(error.has_error() != "valid utf16_le string"_b) << fatal;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF16_LE>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF16_LE>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF16_BE>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::UTF16_BE>(source, dest.data());

					expect(error.has_error() != "valid utf16_be string"_b) << fatal;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF16_BE>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF16_BE>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf8_char_string();

				expect(Scalar<"utf8.char">::validate<true>(source) == "valid utf8_char string"_b) << fatal;

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF32>(source));
					const auto error = Scalar<"utf8.char">::convert<CharsType::UTF32>(source, dest.data());

					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF32>(source)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf8.char">::length<CharsType::UTF32>(source));
					const auto length = Scalar<"utf8.char">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << fatal;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << fatal;

					expect(dest == Scalar<"utf8.char">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << fatal;
				}
			}
		};
	};
}
