#include <unit_test/unit_test.hpp>
#include <chars/icelake.hpp>

#include "gen.hpp"

using namespace gal::prometheus;

namespace
{
	// todo: Simd<"icelake.utf8_char"> / Simd<"icelake.utf8"> / Simd<"utf32">
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.le.icelake"> chars_utf16_le = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string_ascii_only();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::LATIN, std::endian::little>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid latin string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR, std::endian::little>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8, std::endian::little>(source);

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid utf8 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf16_le"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF16_LE, std::endian::little>(source);

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid utf16_le string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
					expect(valid == "valid utf16_le string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
					expect(valid == "valid utf16_le string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf16_be"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF16_BE, std::endian::little>(source);

				using output_type = std::basic_string<char16_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid utf16_be string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
					expect(valid == "valid utf16_be string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
					expect(valid == "valid utf16_be string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_le_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::little>(source) == "valid utf8_char string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32, std::endian::little>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::little>(source, dest.data());
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::little>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::little, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"surrogate pair at the middle of string"_test = []
		{
			constexpr static char16_t source[]
			{
					u"AAA"
					u"Café"
					u"😀"
					u"AAA"
					u"Café"
					u"AAA"
			};
			constexpr static auto source_length = std::ranges::size(source) - 1;
			static_assert(source_length == std::char_traits<char16_t>::length(source));

			"to_utf8_char"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};

			"to_utf32"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32g"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};
		};

		"surrogate pair at the end of string"_test = []
		{
			constexpr static char16_t source[]
			{
					u"AAA"
					u"Café"
					u"AAA"
					u"Café"
					u"AAA"
					u"😀"
			};
			constexpr static auto source_length = std::ranges::size(source) - 1;
			static_assert(source_length == std::char_traits<char16_t>::length(source));

			"to_utf8_char"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};

			"to_utf32"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};
		};

		"surrogate pair at the end of block"_test = []
		{
			constexpr static char16_t source[]
			{
					u"AAA" // 1
					u"BBB" // 2
					u"CCC" // 3
					u"DDD" // 4
					u"EEE" // 5
					u"FFF" // 6
					u"GGG" // 7
					u"HHH" // 8
					u"III" // 9
					u"JJJ" // 10
					u"😀"
					u"KKK"
			};
			constexpr static auto source_length = std::ranges::size(source) - 1;
			static_assert(source_length == std::char_traits<char16_t>::length(source));

			"to_utf8_char"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF8_CHAR,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf8.char">::validate<true>(dest);
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};

			"to_utf32"_test = []
			{
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
					>(source, dest.data());
					expect(result == value(output_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);
					const auto result = Simd<"icelake.utf16">::convert<
						CharsType::UTF32,
						std::endian::little,
						InputProcessPolicy::RETURN_RESULT_TYPE
					>(source, dest.data());
					expect(result.has_error() != "valid utf16 string"_b) << fatal;
					expect(result.count == value(source_length)) << fatal;

					const auto error = Scalar<"utf32">::validate<true>(dest);
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == output_length) << fatal;
				}
			};
		};

		"single surrogate at the middle of string"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xd83d,
						u'C',
						u'C',
						u'a',
						u'f',
						u'é',
						u'D',
						u'E',
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xde00,
						u'C',
						u'C',
						u'a',
						u'f',
						u'é',
						u'D',
						u'E',
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};
		};

		"single surrogate at the end of string"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xd83d,
						'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xde00,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};
		};

		"single surrogate at the end of block"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'A', u'A', u'A', // 1
						u'B', u'B', u'B', // 2
						u'C', u'C', u'C', // 3
						u'D', u'D', u'D', // 4
						u'E', u'E', u'E', // 5
						u'F', u'F', u'F', // 6
						u'G', u'G', u'G', // 7
						u'H', u'H', u'H', // 8
						u'I', u'I', u'I', // 9
						u'J', u'J', u'J', // 10
						0xd83d, 0xd83d, 0xd83d,
						u'K', u'K', u'K',
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());

						expect(result == 0_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());

						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'A', u'A', u'A', // 1
						u'B', u'B', u'B', // 2
						u'C', u'C', u'C', // 3
						u'D', u'D', u'D', // 4
						u'E', u'E', u'E', // 5
						u'F', u'F', u'F', // 6
						u'G', u'G', u'G', // 7
						u'H', u'H', u'H', // 8
						u'I', u'I', u'I', // 9
						u'J', u'J', u'J', // 10
						0xde00, 0xde00, 0xde00,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};
			};
		};

		"mismatch surrogate pair at the middle of string"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xd83d,
						0xd83d,
						u'C',
						u'C',
						u'a',
						u'f',
						u'é',
						u'D',
						u'E',
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xde00,
						0xde00,
						u'C',
						u'C',
						u'a',
						u'f',
						u'é',
						u'D',
						u'E',
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};
		};

		"mismatch single surrogate at the end of string"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xd83d,
						0xd83d,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'C',
						u'a',
						u'f',
						u'é',
						u'A',
						u'B',
						0xde00,
						0xde00,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 6_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 6});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 6_auto) << fatal;
					}
				};
			};
		};

		"mismatch single surrogate at the end of block"_test = []
		{
			"high surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'A', u'A', u'A', // 1
						u'B', u'B', u'B', // 2
						u'C', u'C', u'C', // 3
						u'D', u'D', u'D', // 4
						u'E', u'E', u'E', // 5
						u'F', u'F', u'F', // 6
						u'G', u'G', u'G', // 7
						u'H', u'H', u'H', // 8
						u'I', u'I', u'I', // 9
						u'J', u'J', u'J', // 10
						0xd83d, 0xd83d,
						0xd83d, 0xde00,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};
			};

			"low surrogate"_test = []
			{
				constexpr static char16_t source[]
				{
						u'A', u'A', u'A', // 1
						u'B', u'B', u'B', // 2
						u'C', u'C', u'C', // 3
						u'D', u'D', u'D', // 4
						u'E', u'E', u'E', // 5
						u'F', u'F', u'F', // 6
						u'G', u'G', u'G', // 7
						u'H', u'H', u'H', // 8
						u'I', u'I', u'I', // 9
						u'J', u'J', u'J', // 10
						0xde00, 0xde00,
						0xd83d, 0xde00,
						u'\0',
				};
				constexpr static auto source_length = std::ranges::size(source) - 1;
				static_assert(source_length == std::char_traits<char16_t>::length(source));

				"to_utf8_char"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR>(source);

					using output_type = std::basic_string<char>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF8_CHAR,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf8.char">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf8_char string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};

				"to_utf32"_test = []
				{
					const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32>(source);

					using output_type = std::basic_string<char32_t>;
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::ZERO_IF_ERROR_ELSE_PROCESSED_OUTPUT
						>(source, dest.data());
						expect(result == 0_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
					{
						output_type dest{};
						dest.resize(output_length);
						const auto result = Simd<"icelake.utf16">::convert<
							CharsType::UTF32,
							std::endian::little,
							InputProcessPolicy::RETURN_RESULT_TYPE
						>(source, dest.data());
						expect(result.has_error() == "invalid utf16 string"_b) << fatal;
						expect(result.error == value(ErrorCode::SURROGATE));
						expect(result.count == 30_auto) << fatal;

						const auto error = Scalar<"utf32">::validate<true>({dest.data(), 30});
						expect(error.has_error() != "valid utf32 string"_b) << fatal;
						expect(error.count == 30_auto) << fatal;
					}
				};
			};
		};
	};

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"chars.utf16.be.icelake"> chars_utf16_be = []
	{
		using namespace unit_test;
		using namespace chars;

		constexpr std::size_t trials = 1000;

		"to_latin"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_be_string_ascii_only();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::LATIN, std::endian::big>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::big>(source, dest.data());
					expect(error.has_error() != "valid latin string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::big>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Simd<"icelake.latin">::validate<true>(dest);
					expect(valid == "valid latin string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::LATIN, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8_char"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_be_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8_CHAR, std::endian::big>(source);

				using output_type = std::basic_string<char>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::big>(source, dest.data());
					expect(error.has_error() != "valid utf8_char string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::big>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8.char">::validate<true>(dest);
					expect(valid == "valid utf8_char string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8_CHAR, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf8"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_be_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF8, std::endian::big>(source);

				using output_type = std::basic_string<char8_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::big>(source, dest.data());
					expect(error.has_error() != "valid utf8 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::big>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf8">::validate<true>(dest);
					expect(valid == "valid utf8 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF8, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};

		"to_utf16_le"_test = []
		{
			const auto source = make_random_utf16_be_string();

			expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << fatal;
			const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF16_LE, std::endian::big>(source);

			using output_type = std::basic_string<char16_t>;
			{
				output_type dest{};
				dest.resize(output_length);

				const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::big>(source, dest.data());
				expect(error.has_error() != "valid utf16_le string"_b) << fatal;
				expect(error.count == value(source.size()));

				const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
				expect(valid == "valid utf16_le string"_b) << fatal;

				const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::big>(source);
				expect(dest == ref(result)) << fatal;
			}
			{
				output_type dest{};
				dest.resize(output_length);

				const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
				expect(length == value(dest.size())) << fatal;

				const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::little>(dest);
				expect(valid == "valid utf16_le string"_b) << fatal;

				const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_LE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(dest == ref(result)) << fatal;
			}
		};

		"to_utf16_be"_test = []
		{
			const auto source = make_random_utf16_be_string();

			expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf16 string"_b) << fatal;
			const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF16_BE, std::endian::big>(source);

			using output_type = std::basic_string<char16_t>;
			{
				output_type dest{};
				dest.resize(output_length);

				const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::big>(source, dest.data());
				expect(error.has_error() != "valid utf16_be string"_b) << fatal;
				expect(error.count == value(source.size()));

				const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
				expect(valid == "valid utf16_be string"_b) << fatal;

				const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::big>(source);
				expect(dest == ref(result)) << fatal;
			}
			{
				output_type dest{};
				dest.resize(output_length);

				const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
				expect(length == value(dest.size())) << fatal;

				const auto valid = Simd<"icelake.utf16">::validate<true, std::endian::big>(dest);
				expect(valid == "valid utf16_be string"_b) << fatal;

				const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF16_BE, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
				expect(dest == ref(result)) << fatal;
			}
		};

		"to_utf32"_test = []
		{
			for (std::size_t i = 0; i < trials; ++i)
			{
				const auto source = make_random_utf16_be_string();

				expect(Simd<"icelake.utf16">::validate<true, std::endian::big>(source) == "valid utf32 string"_b) << fatal;
				const auto output_length = Simd<"icelake.utf16">::length<CharsType::UTF32, std::endian::big>(source);

				using output_type = std::basic_string<char32_t>;
				{
					output_type dest{};
					dest.resize(output_length);

					const auto error = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::big>(source, dest.data());
					expect(error.has_error() != "valid utf32 string"_b) << fatal;
					expect(error.count == value(source.size()));

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::big>(source);
					expect(dest == ref(result)) << fatal;
				}
				{
					output_type dest{};
					dest.resize(output_length);

					const auto length = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source, dest.data());
					expect(length == value(dest.size())) << fatal;

					const auto valid = Scalar<"utf32">::validate<true>(dest);
					expect(valid == "valid utf32 string"_b) << fatal;

					const auto result = Simd<"icelake.utf16">::convert<CharsType::UTF32, std::endian::big, InputProcessPolicy::ASSUME_VALID_INPUT>(source);
					expect(dest == ref(result)) << fatal;
				}
			}
		};
	};
}
