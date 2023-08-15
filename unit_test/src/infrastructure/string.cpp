#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace infrastructure;
	using namespace std::string_view_literals;

	GAL_PROMETHEUS_NO_DESTROY suite test_infrastructure_char_array = []
	{
		"char_array"_test = []
		{
			using string_type = GAL_PROMETHEUS_STRING_CHAR_ARRAY("hello world");
			static_assert(string_type::match("hello world"));
			static_assert(string_type::match("hello world"sv));
			static_assert(string_type::as_string_view() == "hello world");

			using wstring_type = GAL_PROMETHEUS_STRING_WCHAR_ARRAY("hello world");
			static_assert(wstring_type::match(L"hello world"));
			static_assert(wstring_type::match(L"hello world"sv));
			static_assert(wstring_type::as_string_view() == L"hello world");

			using u8string_type = GAL_PROMETHEUS_STRING_U8CHAR_ARRAY("hello world");
			static_assert(u8string_type::match(u8"hello world"));
			static_assert(u8string_type::match(u8"hello world"sv));
			static_assert(u8string_type::as_string_view() == u8"hello world");

			using u16string_type = GAL_PROMETHEUS_STRING_U16CHAR_ARRAY("hello world");
			static_assert(u16string_type::match(u"hello world"));
			static_assert(u16string_type::match(u"hello world"sv));
			static_assert(u16string_type::as_string_view() == u"hello world");

			using u32string_type = GAL_PROMETHEUS_STRING_U32CHAR_ARRAY("hello world");
			static_assert(u32string_type::match(U"hello world"));
			static_assert(u32string_type::match(U"hello world"sv));
			static_assert(u32string_type::as_string_view() == U"hello world");
		};

		"char_bilateral_array"_test = []
		{
			using string_type = GAL_PROMETHEUS_STRING_CHAR_BILATERAL_ARRAY("hello", "world");
			static_assert(string_type::match_left("hello"));
			static_assert(string_type::match_left("hello"sv));
			static_assert(string_type::match_right("world"));
			static_assert(string_type::match_right("world"sv));

			using wstring_type = GAL_PROMETHEUS_STRING_WCHAR_BILATERAL_ARRAY("hello", "world");
			static_assert(wstring_type::match_left(L"hello"));
			static_assert(wstring_type::match_left(L"hello"sv));
			static_assert(wstring_type::match_right(L"world"));
			static_assert(wstring_type::match_right(L"world"sv));

			using u8string_type = GAL_PROMETHEUS_STRING_U8CHAR_BILATERAL_ARRAY("hello", "world");
			static_assert(u8string_type::match_left(u8"hello"));
			static_assert(u8string_type::match_left(u8"hello"sv));
			static_assert(u8string_type::match_right(u8"world"));
			static_assert(u8string_type::match_right(u8"world"sv));

			using u16string_type = GAL_PROMETHEUS_STRING_U16CHAR_BILATERAL_ARRAY("hello", "world");
			static_assert(u16string_type::match_left(u"hello"));
			static_assert(u16string_type::match_left(u"hello"sv));
			static_assert(u16string_type::match_right(u"world"));
			static_assert(u16string_type::match_right(u"world"sv));

			using u32string_type = GAL_PROMETHEUS_STRING_U32CHAR_BILATERAL_ARRAY("hello", "world");
			static_assert(u32string_type::match_left(U"hello"));
			static_assert(u32string_type::match_left(U"hello"sv));
			static_assert(u32string_type::match_right(U"world"));
			static_assert(u32string_type::match_right(U"world"sv));
		};

		"char_symmetry_array"_test = []
		{
			using string_type = GAL_PROMETHEUS_STRING_CHAR_SYMMETRY_ARRAY("hello" "world");
			static_assert(string_type::match_left("hello"));
			static_assert(string_type::match_left("hello"sv));
			static_assert(string_type::match_right("world"));
			static_assert(string_type::match_right("world"sv));

			using wstring_type = GAL_PROMETHEUS_STRING_WCHAR_SYMMETRY_ARRAY("hello" "world");
			static_assert(wstring_type::match_left(L"hello"));
			static_assert(wstring_type::match_left(L"hello"sv));
			static_assert(wstring_type::match_right(L"world"));
			static_assert(wstring_type::match_right(L"world"sv));

			using u8string_type = GAL_PROMETHEUS_STRING_U8CHAR_SYMMETRY_ARRAY("hello" "world");
			static_assert(u8string_type::match_left(u8"hello"));
			static_assert(u8string_type::match_left(u8"hello"sv));
			static_assert(u8string_type::match_right(u8"world"));
			static_assert(u8string_type::match_right(u8"world"sv));

			using u16string_type = GAL_PROMETHEUS_STRING_U16CHAR_SYMMETRY_ARRAY("hello" "world");
			static_assert(u16string_type::match_left(u"hello"));
			static_assert(u16string_type::match_left(u"hello"sv));
			static_assert(u16string_type::match_right(u"world"));
			static_assert(u16string_type::match_right(u"world"sv));

			using u32string_type = GAL_PROMETHEUS_STRING_U32CHAR_SYMMETRY_ARRAY("hello" "world");
			static_assert(u32string_type::match_left(U"hello"));
			static_assert(u32string_type::match_left(U"hello"sv));
			static_assert(u32string_type::match_right(U"world"));
			static_assert(u32string_type::match_right(U"world"sv));
		};

		"char_multiple_array"_test = []
		{
			using string_type = GAL_PROMETHEUS_STRING_CHAR_MULTIPLE_ARRAY("hello", "world", "1");
			static_assert(string_type::match<0>("hello"));
			static_assert(string_type::match<0>("hello"sv));
			static_assert(string_type::match<1>("world"));
			static_assert(string_type::match<1>("world"sv));
			static_assert(string_type::match<2>("1"));
			static_assert(string_type::match<2>("1"sv));

			using wstring_type = GAL_PROMETHEUS_STRING_WCHAR_MULTIPLE_ARRAY("hello", "world", "1", "2");
			static_assert(wstring_type::match<0>(L"hello"));
			static_assert(wstring_type::match<0>(L"hello"sv));
			static_assert(wstring_type::match<1>(L"world"));
			static_assert(wstring_type::match<1>(L"world"sv));
			static_assert(wstring_type::match<2>(L"1"));
			static_assert(wstring_type::match<2>(L"1"sv));
			static_assert(wstring_type::match<3>(L"2"));
			static_assert(wstring_type::match<3>(L"2"sv));

			using u8string_type = GAL_PROMETHEUS_STRING_U8CHAR_MULTIPLE_ARRAY("hello", "world", "1");
			static_assert(u8string_type::match<0>(u8"hello"));
			static_assert(u8string_type::match<0>(u8"hello"sv));
			static_assert(u8string_type::match<1>(u8"world"));
			static_assert(u8string_type::match<1>(u8"world"sv));
			static_assert(u8string_type::match<2>(u8"1"));
			static_assert(u8string_type::match<2>(u8"1"sv));

			using u16string_type = GAL_PROMETHEUS_STRING_U16CHAR_MULTIPLE_ARRAY("hello", "world", "1");
			static_assert(u16string_type::match<0>(u"hello"));
			static_assert(u16string_type::match<0>(u"hello"sv));
			static_assert(u16string_type::match<1>(u"world"));
			static_assert(u16string_type::match<1>(u"world"sv));
			static_assert(u16string_type::match<2>(u"1"));
			static_assert(u16string_type::match<2>(u"1"sv));

			using u32string_type = GAL_PROMETHEUS_STRING_U32CHAR_MULTIPLE_ARRAY("hello", "world", "1");
			static_assert(u32string_type::match<0>(U"hello"));
			static_assert(u32string_type::match<0>(U"hello"sv));
			static_assert(u32string_type::match<1>(U"world"));
			static_assert(u32string_type::match<1>(U"world"sv));
			static_assert(u32string_type::match<2>(U"1"));
			static_assert(u32string_type::match<2>(U"1"sv));
		};
	};
}// namespace
