#include <unit_test/unit_test.hpp>
#include <chars/scalar.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf32.scalar"> _ = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_latin"_test = [source = make_random_utf32_string_ascii_only()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::LATIN>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::LATIN>(source, dest.data());

					expect(error.has_error() != "valid latin string"_b) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::LATIN>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::LATIN>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::LATIN, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8_char"_test = [source = make_random_utf32_string()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF8_CHAR>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::UTF8_CHAR>(source, dest.data());

					expect(error.has_error() != "valid utf8_char string"_b) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF8_CHAR>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF8_CHAR>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF8_CHAR, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8"_test = [source = make_random_utf32_string()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF8>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::UTF8>(source, dest.data());

					expect(error.has_error() != "valid utf8 string"_b) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF8>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF8>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF8, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_le"_test = [source = make_random_utf32_string()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF16_LE>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::UTF16_LE>(source, dest.data());

					expect(error.has_error() != "valid utf16_le string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF16_LE>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF16_LE>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF16_LE, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_be"_test = [source = make_random_utf32_string()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF16_BE>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::UTF16_BE>(source, dest.data());

					expect(error.has_error() != "valid utf16_be string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF16_BE>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF16_BE>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF16_BE, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf32"_test = [source = make_random_utf32_string()]
			{
				expect(Scalar<"utf32">::validate<true>(source) == "valid utf32 string"_b) << required;
				expect(Scalar<"utf32">::length<CharsType::UTF32>(source) == value(source.size())) << required;

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF32>(source));
					const auto error = Scalar<"utf32">::convert<CharsType::UTF32>(source, dest.data());

					expect(error.has_error() != "valid utf32 string"_b) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF32>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf32">::length<CharsType::UTF32>(source));
					const auto length = Scalar<"utf32">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf32">::convert<CharsType::UTF32, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};
		}
	};
}
