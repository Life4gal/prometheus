#include <unit_test/unit_test.hpp>
#include <chars/scalar.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.detect_encoding.scalar"> detect_encoding = []
	{
		using namespace chars;

		make_test_detect_encoding<Scalar>();
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.latin.scalar"> latin = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_latin_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF8_CHAR, Scalar, false>(make_random_latin_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF8, Scalar, false>(make_random_latin_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF16_LE, Scalar, false>(make_random_latin_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF16_BE, Scalar, false>(make_random_latin_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF32, Scalar, false>(make_random_latin_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8.char.scalar"> utf8_char = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf8_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8_CHAR, CharsType::LATIN, Scalar>(make_random_utf8_char_string_ascii_only());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8_CHAR, CharsType::UTF8, Scalar>(make_random_utf8_char_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8_CHAR, CharsType::UTF16_LE, Scalar>(make_random_utf8_char_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8_CHAR, CharsType::UTF16_BE, Scalar>(make_random_utf8_char_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8_CHAR, CharsType::UTF32, Scalar>(make_random_utf8_char_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8.scalar"> utf8 = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf8_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8, CharsType::LATIN, Scalar>(make_random_utf8_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8, CharsType::UTF8_CHAR, Scalar>(make_random_utf8_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8, CharsType::UTF16_LE, Scalar>(make_random_utf8_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8, CharsType::UTF16_BE, Scalar>(make_random_utf8_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF8, CharsType::UTF32, Scalar>(make_random_utf8_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.le.scalar"> utf16_le = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf16_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::LATIN, Scalar>(make_random_utf16_le_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF8_CHAR, Scalar>(make_random_utf16_le_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF8, Scalar>(make_random_utf16_le_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF16_BE, Scalar>(make_random_utf16_le_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF32, Scalar>(make_random_utf16_le_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.be.scalar"> utf16_be = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf16_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::LATIN, Scalar>(make_random_utf16_be_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF8_CHAR, Scalar>(make_random_utf16_be_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF8, Scalar>(make_random_utf16_be_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF16_LE, Scalar>(make_random_utf16_be_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF32, Scalar>(make_random_utf16_be_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf32.scalar"> utf32 = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf32_error<Scalar>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::LATIN, Scalar>(make_random_utf32_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF8_CHAR, Scalar>(make_random_utf32_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF8, Scalar>(make_random_utf32_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF16_LE, Scalar>(make_random_utf32_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF16_BE, Scalar>(make_random_utf32_string());
			}
		};
	};
}
