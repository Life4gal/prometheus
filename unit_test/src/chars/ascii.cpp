#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.utility;
import gal.prometheus.chars;

namespace
{
	using namespace gal::prometheus;
	using namespace chars;

	GAL_PROMETHEUS_NO_DESTROY test::suite<"chars.ascii"> _ = []
	{
		using namespace test;

		ignore_pass / "copy_check"_test = []
		{
			std::string identity{};
			identity.reserve(128);
			for (int i = 0; i < 128; ++i) { identity.push_back(utility::char_cast<char>(i)); }

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);
					const auto result = CharConverter<char_map_category_ascii, char_map_category_ascii>{}(origin);

					expect(that % origin == result) << fatal;
				}
			}
		};

		ignore_pass / "move_check"_test = []
		{
			std::string identity{};
			identity.reserve(128);
			for (int i = 0; i < 128; ++i) { identity.push_back(utility::char_cast<char>(i)); }

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);

					auto       copy   = origin;
					const auto result = CharConverter<char_map_category_ascii, char_map_category_ascii>{}(std::move(copy));

					expect(copy.empty() == "moved"_b) << fatal;
					expect(that % origin == result) << fatal;
				}
			}
		};

		ignore_pass / "invalid_char_conversion"_test = []
		{
			constexpr std::string_view text_with_invalid_ascii{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\x80\x81\x82...\xff"};
			constexpr std::string_view text_after_conversion{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789???...?"};
			static_assert(text_with_invalid_ascii.size() == text_after_conversion.size());

			for (std::string::size_type i = 0; i < text_with_invalid_ascii.size(); ++i)
			{
				for (std::string::size_type j = i; j < text_with_invalid_ascii.size(); ++j)
				{
					const auto origin = text_with_invalid_ascii.substr(i, j - i);
					const auto expected = text_after_conversion.substr(i, j - i);

					const auto result	= CharConverter<char_map_category_ascii, char_map_category_ascii>{}(origin);

					expect(that % expected == result) << fatal;
				}
			}
		};
	};
}// namespace
