#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.infrastructure;
import gal.prometheus.chars;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace chars;

	GAL_PROMETHEUS_NO_DESTROY suite test_chars_cp_1252 = []
	{
		"copy_check"_test = []
		{
			std::string identity{};
			identity.reserve(256);
			for (int i = 0; i < 256; ++i) { identity.push_back(infrastructure::char_cast<char>(i)); }

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);
					const auto result = CharConverter<char_map_category_cp_1252, char_map_category_cp_1252>{}(origin);

					expect((origin == result) >> fatal);
				}
			}
		};

		"move_check"_test = []
		{
			std::string identity{};
			identity.reserve(256);
			for (int i = 0; i < 256; ++i) { identity.push_back(infrastructure::char_cast<char>(i)); }

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);

					auto	   copy	  = origin;
					const auto result = CharConverter<char_map_category_cp_1252, char_map_category_cp_1252>{}(std::move(copy));

					expect((copy.empty() == "moved"_b) >> fatal);
					expect((origin == result) >> fatal);
				}
			}
		};
	};
}// namespace
