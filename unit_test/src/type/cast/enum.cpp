#include <boost/ut.hpp>
#include <prometheus/macro.hpp>
#include <prometheus/type/cast/enum.hpp>

using namespace boost::ut;
using namespace gal::prometheus::type::cast;

namespace
{
	enum PlainEnum
	{
		PLAIN_1 = 1 << 1,
		PLAIN_2 = 1 << 2,
		PLAIN_3 = 1 << 3,
	};

	enum PlainContiguousEnum
	{
		PLAIN_CONTIGUOUS_1,
		PLAIN_CONTIGUOUS_2,
		PLAIN_CONTIGUOUS_3,
	};

	enum class ScopedEnum
	{
		_1 = 1 << 1,
		_2 = 1 << 2,
		_3 = 1 << 3,
	};

	enum class ScopedContiguousEnum
	{
		_1,
		_2,
		_3,
	};

	constexpr EnumMeta plain_enum{
			PLAIN_1, "PLAIN_1",//
			PLAIN_2, "PLAIN_2",//
			PLAIN_3, "PLAIN_3" //
	};

	constexpr EnumMeta plain_contiguous_enum{
			PLAIN_CONTIGUOUS_1, "PC1",//
			PLAIN_CONTIGUOUS_2, "PC2",//
			PLAIN_CONTIGUOUS_3, "PC3" //
	};

	constexpr EnumMeta scoped_enum{
			ScopedEnum::_1, "S1",//
			ScopedEnum::_2, "S2",//
			ScopedEnum::_3, "S3" //
	};

	constexpr EnumMeta scoped_contiguous_enum{
			ScopedContiguousEnum::_1, "SC1",//
			ScopedContiguousEnum::_2, "SC2",//
			ScopedContiguousEnum::_3, "SC3" //
	};

	GAL_PROMETHEUS_NO_DESTROY suite test_type_cast_enum = []
	{
		"min_max_contiguous"_test = []
		{
			static_assert(decltype(plain_enum)::size == 3);
			static_assert(plain_enum.min() == PLAIN_1);
			static_assert(plain_enum.max() == PLAIN_3);
			static_assert(not plain_enum.contiguous());

			static_assert(decltype(plain_contiguous_enum)::size == 3);
			static_assert(plain_contiguous_enum.min() == PLAIN_CONTIGUOUS_1);
			static_assert(plain_contiguous_enum.max() == PLAIN_CONTIGUOUS_3);
			static_assert(plain_contiguous_enum.contiguous());

			static_assert(decltype(scoped_enum)::size == 3);
			static_assert(scoped_enum.min() == ScopedEnum::_1);
			static_assert(scoped_enum.max() == ScopedEnum::_3);
			static_assert(not scoped_enum.contiguous());

			static_assert(decltype(scoped_contiguous_enum)::size == 3);
			static_assert(scoped_contiguous_enum.min() == ScopedContiguousEnum::_1);
			static_assert(scoped_contiguous_enum.max() == ScopedContiguousEnum::_3);
			static_assert(scoped_contiguous_enum.contiguous());
		};

		"contains"_test = []
		{
			static_assert(plain_enum.contains(PLAIN_1));
			static_assert(plain_enum.contains(PLAIN_2));
			static_assert(plain_enum.contains(PLAIN_3));

			static_assert(plain_contiguous_enum.contains(PLAIN_CONTIGUOUS_1));
			static_assert(plain_contiguous_enum.contains(PLAIN_CONTIGUOUS_2));
			static_assert(plain_contiguous_enum.contains(PLAIN_CONTIGUOUS_3));

			static_assert(scoped_enum.contains(ScopedEnum::_1));
			static_assert(scoped_enum.contains(ScopedEnum::_2));
			static_assert(scoped_enum.contains(ScopedEnum::_3));

			static_assert(scoped_contiguous_enum.contains(ScopedContiguousEnum::_1));
			static_assert(scoped_contiguous_enum.contains(ScopedContiguousEnum::_2));
			static_assert(scoped_contiguous_enum.contains(ScopedContiguousEnum::_3));
		};

		"operator[] by_value"_test = []
		{
			static_assert(plain_enum[PLAIN_1] == "PLAIN_1");
			static_assert(plain_enum[PLAIN_2] == "PLAIN_2");
			static_assert(plain_enum[PLAIN_3] == "PLAIN_3");

			static_assert(plain_contiguous_enum[PLAIN_CONTIGUOUS_1] == "PC1");
			static_assert(plain_contiguous_enum[PLAIN_CONTIGUOUS_2] == "PC2");
			static_assert(plain_contiguous_enum[PLAIN_CONTIGUOUS_3] == "PC3");

			static_assert(scoped_enum[ScopedEnum::_1] == "S1");
			static_assert(scoped_enum[ScopedEnum::_2] == "S2");
			static_assert(scoped_enum[ScopedEnum::_3] == "S3");

			static_assert(scoped_contiguous_enum[ScopedContiguousEnum::_1] == "SC1");
			static_assert(scoped_contiguous_enum[ScopedContiguousEnum::_2] == "SC2");
			static_assert(scoped_contiguous_enum[ScopedContiguousEnum::_3] == "SC3");
		};

		"operator[] by_name"_test = []
		{
			static_assert(plain_enum["PLAIN_1"] == PLAIN_1);
			static_assert(plain_enum["PLAIN_2"] == PLAIN_2);
			static_assert(plain_enum["PLAIN_3"] == PLAIN_3);

			static_assert(plain_contiguous_enum["PC1"] == PLAIN_CONTIGUOUS_1);
			static_assert(plain_contiguous_enum["PC2"] == PLAIN_CONTIGUOUS_2);
			static_assert(plain_contiguous_enum["PC3"] == PLAIN_CONTIGUOUS_3);

			static_assert(scoped_enum["S1"] == ScopedEnum::_1);
			static_assert(scoped_enum["S2"] == ScopedEnum::_2);
			static_assert(scoped_enum["S3"] == ScopedEnum::_3);

			static_assert(scoped_contiguous_enum["SC1"] == ScopedContiguousEnum::_1);
			static_assert(scoped_contiguous_enum["SC2"] == ScopedContiguousEnum::_2);
			static_assert(scoped_contiguous_enum["SC3"] == ScopedContiguousEnum::_3);
		};

		"at by_value"_test = []
		{
			static_assert(plain_enum.at(PLAIN_1) == "PLAIN_1");
			static_assert(plain_enum.at(PLAIN_2) == "PLAIN_2");
			static_assert(plain_enum.at(PLAIN_3) == "PLAIN_3");
			expect((throws<std::out_of_range>([] { (void)plain_enum.at(static_cast<PlainEnum>(42)); })) >> fatal);
			static_assert(plain_enum.at(PLAIN_1, "42!") == "PLAIN_1");
			static_assert(plain_enum.at(PLAIN_2, "42!") == "PLAIN_2");
			static_assert(plain_enum.at(PLAIN_3, "42!") == "PLAIN_3");
			static_assert(plain_enum.at(static_cast<PlainEnum>(42), "42!") == "42!");

			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_1) == "PC1");
			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_2) == "PC2");
			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_3) == "PC3");
			expect((throws<std::out_of_range>([] { (void)plain_contiguous_enum.at(static_cast<PlainContiguousEnum>(42)); })) >> fatal);
			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_1, "42!") == "PC1");
			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_2, "42!") == "PC2");
			static_assert(plain_contiguous_enum.at(PLAIN_CONTIGUOUS_3, "42!") == "PC3");
			static_assert(plain_contiguous_enum.at(static_cast<PlainContiguousEnum>(42), "42!") == "42!");

			static_assert(scoped_enum.at(ScopedEnum::_1) == "S1");
			static_assert(scoped_enum.at(ScopedEnum::_2) == "S2");
			static_assert(scoped_enum.at(ScopedEnum::_3) == "S3");
			expect((throws<std::out_of_range>([] { (void)scoped_enum.at(static_cast<ScopedEnum>(42)); })) >> fatal);
			static_assert(scoped_enum.at(ScopedEnum::_1, "42!") == "S1");
			static_assert(scoped_enum.at(ScopedEnum::_2, "42!") == "S2");
			static_assert(scoped_enum.at(ScopedEnum::_3, "42!") == "S3");
			static_assert(scoped_enum.at(static_cast<ScopedEnum>(42), "42!") == "42!");

			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_1) == "SC1");
			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_2) == "SC2");
			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_3) == "SC3");
			expect((throws<std::out_of_range>([] { (void)scoped_contiguous_enum.at(static_cast<ScopedContiguousEnum>(42)); })) >> fatal);
			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_1, "42!") == "SC1");
			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_2, "42!") == "SC2");
			static_assert(scoped_contiguous_enum.at(ScopedContiguousEnum::_3, "42!") == "SC3");
			static_assert(scoped_contiguous_enum.at(static_cast<ScopedContiguousEnum>(42), "42!") == "42!");
		};

		"at by_name"_test = []
		{
			static_assert(plain_enum.at("PLAIN_1") == PLAIN_1);
			static_assert(plain_enum.at("PLAIN_2") == PLAIN_2);
			static_assert(plain_enum.at("PLAIN_3") == PLAIN_3);
			expect((throws<std::out_of_range>([] { (void)plain_enum.at("42!"); })) >> fatal);
			static_assert(plain_enum.at("PLAIN_1", PLAIN_1) == PLAIN_1);
			static_assert(plain_enum.at("PLAIN_2", PLAIN_1) == PLAIN_2);
			static_assert(plain_enum.at("PLAIN_3", PLAIN_1) == PLAIN_3);
			static_assert(plain_enum.at("42!", PLAIN_1) == PLAIN_1);

			static_assert(plain_contiguous_enum.at("PC1") == PLAIN_CONTIGUOUS_1);
			static_assert(plain_contiguous_enum.at("PC2") == PLAIN_CONTIGUOUS_2);
			static_assert(plain_contiguous_enum.at("PC3") == PLAIN_CONTIGUOUS_3);
			expect((throws<std::out_of_range>([] { (void)plain_contiguous_enum.at("42!"); })) >> fatal);
			static_assert(plain_contiguous_enum.at("PC1", PLAIN_CONTIGUOUS_1) == PLAIN_CONTIGUOUS_1);
			static_assert(plain_contiguous_enum.at("PC2", PLAIN_CONTIGUOUS_1) == PLAIN_CONTIGUOUS_2);
			static_assert(plain_contiguous_enum.at("PC3", PLAIN_CONTIGUOUS_1) == PLAIN_CONTIGUOUS_3);
			static_assert(plain_contiguous_enum.at("42!", PLAIN_CONTIGUOUS_1) == PLAIN_CONTIGUOUS_1);

			static_assert(scoped_enum.at("S1") == ScopedEnum::_1);
			static_assert(scoped_enum.at("S2") == ScopedEnum::_2);
			static_assert(scoped_enum.at("S3") == ScopedEnum::_3);
			expect((throws<std::out_of_range>([] { (void)scoped_enum.at("42!"); })) >> fatal);
			static_assert(scoped_enum.at("S1", ScopedEnum::_1) == ScopedEnum::_1);
			static_assert(scoped_enum.at("S2", ScopedEnum::_1) == ScopedEnum::_2);
			static_assert(scoped_enum.at("S3", ScopedEnum::_1) == ScopedEnum::_3);
			static_assert(scoped_enum.at("42!", ScopedEnum::_1) == ScopedEnum::_1);

			static_assert(scoped_contiguous_enum.at("SC1") == ScopedContiguousEnum::_1);
			static_assert(scoped_contiguous_enum.at("SC2") == ScopedContiguousEnum::_2);
			static_assert(scoped_contiguous_enum.at("SC3") == ScopedContiguousEnum::_3);
			expect((throws<std::out_of_range>([] { (void)scoped_contiguous_enum.at("42!"); })) >> fatal);
			static_assert(scoped_contiguous_enum.at("SC1", ScopedContiguousEnum::_1) == ScopedContiguousEnum::_1);
			static_assert(scoped_contiguous_enum.at("SC2", ScopedContiguousEnum::_1) == ScopedContiguousEnum::_2);
			static_assert(scoped_contiguous_enum.at("SC3", ScopedContiguousEnum::_1) == ScopedContiguousEnum::_3);
			static_assert(scoped_contiguous_enum.at("42!", ScopedContiguousEnum::_1) == ScopedContiguousEnum::_1);
		};
	};
}
