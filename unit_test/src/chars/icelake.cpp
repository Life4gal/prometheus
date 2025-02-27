#include <unit_test/unit_test.hpp>
#include <chars/icelake.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

#if defined(GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED)

namespace
{
	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.latin.icelake"> latin = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_latin_error<Icelake>();
		};

		constexpr std::size_t trials = 1000;

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF8_CHAR, Icelake, false>(make_random_latin_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF8, Icelake, false>(make_random_latin_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF16_LE, Icelake, false>(make_random_latin_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF16_BE, Icelake, false>(make_random_latin_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::LATIN, CharsType::UTF32, Icelake, false>(make_random_latin_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.le.icelake"> utf16_le = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf16_error<Icelake>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::LATIN, Icelake>(make_random_utf16_le_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF8_CHAR, Icelake>(make_random_utf16_le_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF8, Icelake>(make_random_utf16_le_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF16_BE, Icelake>(make_random_utf16_le_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_LE, CharsType::UTF32, Icelake>(make_random_utf16_le_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.be.icelake"> utf16_be = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf16_error<Icelake>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::LATIN, Icelake>(make_random_utf16_be_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF8_CHAR, Icelake>(make_random_utf16_be_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF8, Icelake>(make_random_utf16_be_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF16_LE, Icelake>(make_random_utf16_be_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF16_BE, CharsType::UTF32, Icelake>(make_random_utf16_be_string());
			}
		};
	};

	[[maybe_unused]] GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf32.icelake"> utf32 = []
	{
		using namespace unit_test;
		using namespace chars;

		"error"_test = []
		{
			make_test_utf32_error<Icelake>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::LATIN, Icelake>(make_random_utf32_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF8_CHAR, Icelake>(make_random_utf32_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF8, Icelake>(make_random_utf32_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF16_LE, Icelake>(make_random_utf32_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<CharsType::UTF32, CharsType::UTF16_BE, Icelake>(make_random_utf32_string());
			}
		};
	};
}

#endif
