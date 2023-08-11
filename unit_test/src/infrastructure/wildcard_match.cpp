#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace infrastructure;
	using namespace infrastructure::literals;

	struct point
	{
		int x;
		int y;

		constexpr auto operator==(const point& other) const noexcept -> bool { return x == other.x and y == other.y; }
	};

	template<std::size_t N>
	struct my_point_container
	{
		std::array<point, N> points{};

		[[nodiscard]] constexpr auto begin() const noexcept { return std::cbegin(points); }

		[[nodiscard]] constexpr auto end() const noexcept { return std::cend(points); }
	};
}

template<>
struct infrastructure::wildcard_type<point>
{
	using value_type = point;

	// standard
	value_type anything{10, 10};
	value_type single{20, 20};
	value_type escape{30, 30};

	// extended
	value_type set_open{40, 40};
	value_type set_close{50, 50};
	value_type set_not{60, 60};

	value_type alt_open{70, 70};
	value_type alt_close{80, 80};
	value_type alt_or{90, 90};
};

namespace
{
	GAL_PROMETHEUS_NO_DESTROY suite test_infrastructure_wildcard_match = []
	{
		"basic_test"_test = []
		{
			{
				constexpr char pattern1[] = "";
				constexpr char pattern2[] = R"(\)";

				static_assert(match("", pattern1));
				static_assert(match("", pattern2));

				static_assert(not match("we don't care what's here", pattern1));
				static_assert(not match("we don't care what's here", pattern2));
			}
			{
				constexpr char pattern1[] = "A";
				constexpr char pattern2[] = R"(A\)";
				constexpr char pattern3[] = R"(\A)";
				constexpr char pattern4[] = "[A]";
				constexpr char pattern5[] = "(A)";
				constexpr char pattern6[] = R"((\A))";
				constexpr char pattern7[] = R"(([A]))";

				static_assert(match("A", pattern1));
				static_assert(match("A", pattern2));
				static_assert(match("A", pattern3));
				static_assert(match("A", pattern4));
				static_assert(match("A", pattern5));
				static_assert(match("A", pattern6));
				static_assert(match("A", pattern7));

				static_assert(not match("", pattern1));
				static_assert(not match("", pattern2));
				static_assert(not match("", pattern3));
				static_assert(not match("", pattern4));
				static_assert(not match("", pattern5));
				static_assert(not match("", pattern6));
				static_assert(not match("", pattern7));

				static_assert(not match("a", pattern1));
				static_assert(not match("a", pattern2));
				static_assert(not match("a", pattern3));
				static_assert(not match("a", pattern4));
				static_assert(not match("a", pattern5));
				static_assert(not match("a", pattern6));
				static_assert(not match("a", pattern7));

				static_assert(not match("AA", pattern1));
				static_assert(not match("AA", pattern2));
				static_assert(not match("AA", pattern3));
				static_assert(not match("AA", pattern4));
				static_assert(not match("AA", pattern5));
				static_assert(not match("AA", pattern6));
				static_assert(not match("AA", pattern7));

				static_assert(not match("we don't care what's here", pattern1));
				static_assert(not match("we don't care what's here", pattern2));
				static_assert(not match("we don't care what's here", pattern3));
				static_assert(not match("we don't care what's here", pattern4));
				static_assert(not match("we don't care what's here", pattern5));
				static_assert(not match("we don't care what's here", pattern6));
				static_assert(not match("we don't care what's here", pattern7));
			}
			{
				constexpr char pattern1[] = "Hello!";
				constexpr char pattern2[] = R"(Hello!\)";
				constexpr char pattern3[] = R"(\H\e\l\l\o\!)";
				constexpr char pattern4[] = "[H][e][l][l][o]!";
				constexpr char pattern5[] = "(Hello!)";
				constexpr char pattern6[] = R"((\H\e\l\l\o\!))";
				constexpr char pattern7[] = R"(([H][e][l][l][o]!))";

				static_assert(match("Hello!", pattern1));
				static_assert(match("Hello!", pattern2));
				static_assert(match("Hello!", pattern3));
				static_assert(match("Hello!", pattern4));
				static_assert(match("Hello!", pattern5));
				static_assert(match("Hello!", pattern6));
				static_assert(match("Hello!", pattern7));

				static_assert(not match("", pattern1));
				static_assert(not match("", pattern2));
				static_assert(not match("", pattern3));
				static_assert(not match("", pattern4));
				static_assert(not match("", pattern5));
				static_assert(not match("", pattern6));
				static_assert(not match("", pattern7));

				static_assert(not match("Hello!!", pattern1));
				static_assert(not match("Hello!!", pattern2));
				static_assert(not match("Hello!!", pattern3));
				static_assert(not match("Hello!!", pattern4));
				static_assert(not match("Hello!!", pattern5));
				static_assert(not match("Hello!!", pattern6));
				static_assert(not match("Hello!!", pattern7));

				static_assert(not match("Hello!Hello!", pattern1));
				static_assert(not match("Hello!Hello!", pattern2));
				static_assert(not match("Hello!Hello!", pattern3));
				static_assert(not match("Hello!Hello!", pattern4));
				static_assert(not match("Hello!Hello!", pattern5));
				static_assert(not match("Hello!Hello!", pattern6));
				static_assert(not match("Hello!Hello!", pattern7));
			}
			{
				constexpr char pattern1[] = "*";
				constexpr char pattern2[] = R"(*\)";
				constexpr char pattern3[] = R"(\*)";
				constexpr char pattern4[] = "[*]";
				constexpr char pattern5[] = "(*)";
				constexpr char pattern6[] = R"((\*))";
				constexpr char pattern7[] = "([*])";

				static_assert(match("", pattern1));
				static_assert(match("", pattern2));
				static_assert(not match("", pattern3));
				static_assert(not match("", pattern4));
				static_assert(match("", pattern5));
				static_assert(not match("", pattern6));
				static_assert(not match("", pattern7));

				static_assert(match("*", pattern1));
				static_assert(match("*", pattern2));
				static_assert(match("*", pattern3));
				static_assert(match("*", pattern4));
				// matched without '\0'
				static_assert(not match("*", pattern5));
				static_assert(match("*", pattern6));
				static_assert(match("*", pattern7));

				static_assert(match("we don't care what's here", pattern1));
				static_assert(match("we don't care what's here", pattern2));
				static_assert(not match("we don't care what's here", pattern3));
				static_assert(not match("we don't care what's here", pattern4));
				// matched without '\0'
				static_assert(not match("we don't care what's here", pattern5));
				static_assert(not match("we don't care what's here", pattern6));
				static_assert(not match("we don't care what's here", pattern7));
			}
			{
				constexpr char pattern1[] = "?";
				constexpr char pattern2[] = R"(?\)";
				constexpr char pattern3[] = R"(\?)";
				constexpr char pattern4[] = "[?]";
				constexpr char pattern5[] = "(?)";
				constexpr char pattern6[] = R"((\?))";
				constexpr char pattern7[] = R"([?])";

				static_assert(match("A", pattern1));
				static_assert(match("A", pattern2));
				static_assert(not match("A", pattern3));
				static_assert(not match("A", pattern4));
				static_assert(match("A", pattern5));
				static_assert(not match("A", pattern6));
				static_assert(not match("A", pattern7));

				static_assert(match("a", pattern1));
				static_assert(match("a", pattern2));
				static_assert(not match("a", pattern3));
				static_assert(not match("a", pattern4));
				static_assert(match("a", pattern5));
				static_assert(not match("a", pattern6));
				static_assert(not match("a", pattern7));

				static_assert(match("?", pattern1));
				static_assert(match("?", pattern2));
				static_assert(match("?", pattern3));
				static_assert(match("?", pattern4));
				static_assert(match("?", pattern5));
				static_assert(match("?", pattern6));
				static_assert(match("?", pattern7));

				static_assert(not match("", pattern1));
				static_assert(not match("", pattern2));
				static_assert(not match("", pattern3));
				static_assert(not match("", pattern4));
				static_assert(not match("", pattern5));
				static_assert(not match("", pattern6));
				static_assert(not match("", pattern7));

				static_assert(not match("we don't care what's here", pattern1));
				static_assert(not match("we don't care what's here", pattern2));
				static_assert(not match("we don't care what's here", pattern3));
				static_assert(not match("we don't care what's here", pattern4));
				static_assert(not match("we don't care what's here", pattern5));
				static_assert(not match("we don't care what's here", pattern6));
				static_assert(not match("we don't care what's here", pattern7));
			}
			{
				constexpr char pattern1[] = R"(\\\* *\? \*\\)";
				constexpr char pattern2[] = R"([\][*] *[?] [*][\])";

				static_assert(match(R"(\* Hello? *\)", pattern1));
				static_assert(match(R"(\* Hello? *\)", pattern2));

				static_assert(match(R"(\* Hi? *\)", pattern1));
				static_assert(match(R"(\* Hi? *\)", pattern2));

				static_assert(match(R"(\* ? *\)", pattern1));
				static_assert(match(R"(\* ? *\)", pattern2));

				static_assert(not match(R"(\* Hello! *\)", pattern1));
				static_assert(not match(R"(\* Hello! *\)", pattern2));

				static_assert(not match(R"(* Hello? *\)", pattern1));
				static_assert(not match(R"(* Hello? *\)", pattern2));

				static_assert(not match(R"(\ Hello? *\)", pattern1));
				static_assert(not match(R"(\ Hello? *\)", pattern2));

				static_assert(not match(R"( Hello? *\)", pattern1));
				static_assert(not match(R"( Hello? *\)", pattern2));

				constexpr wchar_t  pattern_wc[]  = L"H?llo,*W*!";
				constexpr char8_t  pattern_u8[]  = u8"H?llo,*W*!";
				constexpr char16_t pattern_u16[] = u"H?llo,*W*!";
				constexpr char32_t pattern_u32[] = U"H?llo,*W*!";

				static_assert(match(L"Hello, World!", pattern_wc));
				static_assert(match(u8"Hello, World!", pattern_u8));
				static_assert(match(u"Hello, World!", pattern_u16));
				static_assert(match(U"Hello, World!", pattern_u32));
			}
			{
				static_assert(match("aaa", "a[abc]a"));
				static_assert(not match("aaa", "a[bcd]a"));
				static_assert(not match("aaa", "a[a]]a"));
				static_assert(match("aa]a", "a[a]]a"));
				static_assert(match("aaa", "a[]abc]a"));
				static_assert(match("aaa", "a[[a]a"));
				static_assert(match("a[a", "a[[a]a"));
				static_assert(match("a]a", "a[]]a"));
				static_assert(not match("aa", "a[]a"));
				static_assert(match("a[]a", "a[]a"));

				static_assert(not match("aaa", "a[!a]a"));
				static_assert(match("aaa", "a[!b]a"));
				static_assert(not match("aaa", "a[b!b]a"));
				static_assert(match("a!a", "a[b!b]a"));
				static_assert(not match("a!a", "a[!]a"));
				static_assert(match("a[!]a", "a[!]a"));
			}
			{
				static_assert(match("aXb", "a(X|Y)b"));
				static_assert(match("aYb", "a(X|Y)b"));
				static_assert(not match("aZb", "a(X|Y)b"));
				static_assert(match("aXb", "(a(X|Y)b|c)"));
				static_assert(not match("a", "a|b"));
				static_assert(match("a|b", "a|b"));
				static_assert(match("(aa", "(a(a|b)"));
				static_assert(not match("a(a", "(a(a|b)"));
				static_assert(match("a(a", "(a[(]a|b)"));
				static_assert(match("aa", "a()a"));
				static_assert(match("", "(abc|)"));
			}
		};

		"matcher"_test = []
		{
			static_assert(make_wildcard_matcher("H?llo,*W*!")("Hello, World!"));
			static_assert(make_wildcard_matcher("H_llo,%W%!", {'%', '_', '\\'})("Hello, World!"));

			auto my_equal_to = [](const int num, const auto character) -> bool { return num + 48 == static_cast<int>(character); };

			constexpr std::string_view    str1{"12*5?"};
			constexpr std::wstring_view   str2{L"12*5?"};
			constexpr std::u8string_view  str3{u8"12*5?"};
			constexpr std::u16string_view str4{u"12*5?"};
			constexpr std::u32string_view str5{U"12*5?"};

			static_assert(make_wildcard_matcher(str1, my_equal_to)(std::array{1, 2, 3, 4, 5, 6}));
			static_assert(make_wildcard_matcher(str2, my_equal_to)(std::array{1, 2, 3, 4, 5, 6}));
			static_assert(make_wildcard_matcher(str3, my_equal_to)(std::array{1, 2, 3, 4, 5, 6}));
			static_assert(make_wildcard_matcher(str4, my_equal_to)(std::array{1, 2, 3, 4, 5, 6}));
			static_assert(make_wildcard_matcher(str5, my_equal_to)(std::array{1, 2, 3, 4, 5, 6}));
		};

		"literal"_test = []
		{
			static_assert("12*5?"_wm("123456"));
			static_assert(L"12*5?"_wm(L"123456"));
			static_assert(u8"12*5?"_wm(u8"123456"));
			static_assert(u"12*5?"_wm(u"123456"));
			static_assert(U"12*5?"_wm(U"123456"));
		};

		"custom_point"_test = []
		{
			constexpr my_point_container<10> container1 =
			{{{
					{1, 10},
					// single
					{20, 20},
					{2, 20},
					{3, 30},
					// anything,
					{10, 10},
					{4, 40},
					// escape
					{30, 30},
					// escaped by escape, does not mean `anything`
					{10, 10},
					{5, 50},
					{6, 60},
			}}};

			constexpr my_point_container<13> container2 =
			{{{{10, 1},
				{1234, 5678},
				{20, 2},
				{30, 3},
				{1234, 5678},
				{1234, 5678},
				{1234, 5678},
				{1234, 5678},
				{1234, 5678},
				{40, 4},
				{100, 1},
				{50, 5},
				{60, 6}}}};

			constexpr auto point_compare = [](const point& p1, const point& p2) constexpr -> bool { return p1.x * p1.y == p2.x * p2.y; };

			static_assert(make_wildcard_matcher(container1, point_compare)(container2));
		};
	};
}
