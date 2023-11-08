#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.utility;
import gal.prometheus.chars;

namespace
{
	using namespace gal::prometheus;
	using namespace chars;

	GAL_PROMETHEUS_NO_DESTROY test::suite<"chars.cp_1252"> _ = []
	{
		using namespace test;

		ignore_pass / "copy_check"_test = []
		{
			std::string identity{};
			identity.reserve(256);
			for (int i = 1; i < 256; ++i) { identity.push_back(utility::char_cast<char>(i)); }
			identity.push_back('\0');

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);
					const auto result = CharConverter<char_map_category_cp_1252, char_map_category_cp_1252>{}(origin);

					expect(that % origin == result) << fatal;
				}
			}
		};

		ignore_pass / "move_check"_test = []
		{
			std::string identity{};
			identity.reserve(256);
			for (int i = 1; i < 256; ++i) { identity.push_back(utility::char_cast<char>(i)); }
			identity.push_back('\0');

			for (std::string::size_type i = 0; i < identity.size(); ++i)
			{
				for (std::string::size_type j = i; j < identity.size(); ++j)
				{
					const auto origin = identity.substr(i, j - i);

					auto	   copy	  = origin;
					const auto result = CharConverter<char_map_category_cp_1252, char_map_category_cp_1252>{}(std::move(copy));

					expect(copy.empty() == "moved"_b) << fatal;
					expect(that % origin == result) << fatal;
				}
			}
		};
	};
}// namespace
