#include <unit_test/unit_test.hpp>
#include <chars/scalar.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.le.scalar"> chars_utf16_le = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_latin"_test = [source = make_random_utf16_le_string_ascii_only()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::LATIN, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::LATIN, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid latin string"_b) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::LATIN, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::LATIN, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::LATIN, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::LATIN, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8_char"_test = [source = make_random_utf16_le_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8_CHAR, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid utf8_char string"_b) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8_CHAR, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8"_test = [source = make_random_utf16_le_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF8, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid utf8 string"_b) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF8, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_le"_test = [source = make_random_utf16_le_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << required;
				expect(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::little>(source) == value(source.size())) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid utf16_le string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_be"_test = [source = make_random_utf16_le_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << required;
				expect(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::little>(source) == value(source.size())) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid utf16_be string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf32"_test = [source = make_random_utf16_le_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::little>(source) == "valid utf8_char string"_b) << required;

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF32, std::endian::little>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF32, std::endian::little>(source, dest.data());

					expect(error.has_error() != "valid utf32 string"_b) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF32, std::endian::little>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF32, std::endian::little>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF32, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF32, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};
		}
	};

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.be.scalar"> chars_utf16_be = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;
		for (std::size_t i = 0; i < trials; ++i)
		{
			"to_latin"_test = [source = make_random_utf16_be_string_ascii_only()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::LATIN, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::LATIN, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid latin string"_b) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::LATIN, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::LATIN, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::LATIN, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"latin">::validate<true>(dest) == "valid latin string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::LATIN, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8_char"_test = [source = make_random_utf16_be_string(0, 10)]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8_CHAR, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid utf8_char string"_b) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8_CHAR, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8.char">::validate<true>(dest) == "valid utf8_char string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8_CHAR, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf8"_test = [source = make_random_utf16_be_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << required;

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF8, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid utf8 string"_b) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF8, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF8, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf8">::validate<true>(dest) == "valid utf8 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF8, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_le"_test = [source = make_random_utf16_be_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << required;
				expect(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::big>(source) == value(source.size())) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid utf16_le string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_LE, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::little>(dest) == "valid utf16_le string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_LE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf16_be"_test = [source = make_random_utf16_be_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << required;
				expect(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::big>(source) == value(source.size())) << required;

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid utf16_be string"_b) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF16_BE, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf16">::validate<true, std::endian::big>(dest) == "valid utf16_be string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF16_BE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};

			"to_utf32"_test = [source = make_random_utf16_be_string()]
			{
				expect(Scalar<"utf16">::validate<true, std::endian::big>(source) == "valid utf32 string"_b) << required;

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF32, std::endian::big>(source));
					const auto error = Scalar<"utf16">::convert<CharsType::UTF32, std::endian::big>(source, dest.data());

					expect(error.has_error() != "valid utf32 string"_b) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF32, std::endian::big>(source)) << required;
				}
				{
					output_type dest{};
					dest.resize(Scalar<"utf16">::length<CharsType::UTF32, std::endian::big>(source));
					const auto length = Scalar<"utf16">::convert<CharsType::UTF32, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());

					expect(length == value(dest.size())) << required;
					expect(Scalar<"utf32">::validate<true>(dest) == "valid utf32 string"_b) << required;

					expect(dest == Scalar<"utf16">::convert<CharsType::UTF32, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source)) << required;
				}
			};
		}
	};
}
