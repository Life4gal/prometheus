#include <unit_test/unit_test.hpp>
#include <chars/scalar.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf8.scalar"> _ = []
	{
		using namespace unit_test;
		using namespace chars;

		using source_type = Scalar<"utf8">;

		"error"_test = []
		{
			make_test_utf8_error<source_type>();
		};

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"latin">>(make_random_utf8_string_ascii_only());
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"utf8.char">>(make_random_utf8_string());
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"utf8">>(make_random_utf8_string());
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"utf16.le">>(make_random_utf8_string());
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"utf16.be">>(make_random_utf8_string());
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				make_test<source_type, Scalar<"utf32">>(make_random_utf8_string());
			}
		};
	};
}
