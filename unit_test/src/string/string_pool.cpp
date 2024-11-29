#include <utility>
#include <coroutine>

#include <unit_test/unit_test.hpp>
// string::string_pool
#include <string/string.hpp>
// numeric::random
#include <numeric/numeric.hpp>

using namespace gal::prometheus;

namespace
{
	#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
	// see https://developercommunity.visualstudio.com/t/Nested-template-lambda-accidentally-ig/10802570
	#define WORKAROUND_MSVC_C3688 using namespace unit_test;
	#else
	#define WORKAROUND_MSVC_C3688
	#endif

	template<typename CharType>
	[[nodiscard]] constexpr auto make_random_string(const std::size_t length) noexcept -> std::basic_string<CharType>
	{
		numeric::Random random{};

		std::basic_string<CharType> result{};
		result.reserve(length);

		std::ranges::generate_n(
			std::back_inserter(result),
			length,
			[&random]() noexcept -> CharType
			{
				return static_cast<CharType>(random.get<std::uint32_t>());
			}
		);
		return result;
	}

	using namespace gal::prometheus;
	using namespace string;

	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"string.string_pool"> _ = []
	{
		using namespace unit_test;

		// const auto old_level = std::exchange(config().output_level, OutputLevel::NONE);

		const auto test_pool = []<typename CharType, bool IsNullTerminate>() noexcept -> void
		{
			WORKAROUND_MSVC_C3688

			"copy"_test = []
			{
				using pool_type = StringPool<CharType, IsNullTerminate>;

				constexpr auto length = 1000;
				expect(length + IsNullTerminate < value(pool_type::default_block_initial_size));

				pool_type p1{};
				std::ignore = p1.add(make_random_string<CharType>(p1.block_initial_size()));
				std::ignore = p1.add(make_random_string<CharType>(length));
				expect(p1.size() == 2_ull) << fatal;

				pool_type p2{};
				std::ignore = p2.add(make_random_string<CharType>(length));
				std::ignore = p2.add(make_random_string<CharType>(p2.block_initial_size()));
				expect(p2.size() == 2_ull) << fatal;

				pool_type p3{};
				std::ignore = p3.add(make_random_string<CharType>(length));
				std::ignore = p3.add(make_random_string<CharType>(length));
				expect(p3.size() == 1_ull) << fatal;

				pool_type p{p1, p2, p3};
				expect(p1.size() == 2_ull) << fatal;
				expect(p2.size() == 2_ull) << fatal;
				expect(p3.size() == 1_ull) << fatal;
				expect(p.size() == 5_ull) << fatal;

				p.join(p1, p2, p3);
				expect(p1.size() == 2_ull) << fatal;
				expect(p2.size() == 2_ull) << fatal;
				expect(p3.size() == 1_ull) << fatal;
				expect(p.size() == 10_ull) << fatal;
			};

			"move"_test = []
			{
				using pool_type = StringPool<CharType, IsNullTerminate>;

				constexpr auto length = 1000;
				expect(length + IsNullTerminate < value(pool_type::default_block_initial_size));

				pool_type p1{};
				std::ignore = p1.add(make_random_string<CharType>(p1.block_initial_size()));
				std::ignore = p1.add(make_random_string<CharType>(length));
				expect(p1.size() == 2_ull) << fatal;
				pool_type p1_copy{p1};

				pool_type p2{};
				std::ignore = p2.add(make_random_string<CharType>(length));
				std::ignore = p2.add(make_random_string<CharType>(p2.block_initial_size()));
				expect(p2.size() == 2_ull) << fatal;
				pool_type p2_copy{p2};

				pool_type p3{};
				std::ignore = p3.add(make_random_string<CharType>(length));
				std::ignore = p3.add(make_random_string<CharType>(length));
				expect((length + IsNullTerminate) * 2 < value(pool_type::default_block_initial_size)) << fatal;
				expect(p3.size() == 1_ull) << fatal;
				pool_type p3_copy{p3};

				pool_type p{std::move(p1), std::move(p2), std::move(p3)};
				expect(p1.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p2.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p3.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p.size() == 5_ull) << fatal;

				p.join(std::move(p1_copy), std::move(p2_copy), std::move(p3_copy));
				expect(p1_copy.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p2_copy.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p3_copy.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p.size() == 10_ull) << fatal;
			};

			"copy_and_move"_test = []
			{
				using pool_type = StringPool<CharType, IsNullTerminate>;

				constexpr auto length = 1000;
				expect(length + IsNullTerminate < value(pool_type::default_block_initial_size));

				pool_type p1{};
				std::ignore = p1.add(make_random_string<CharType>(p1.block_initial_size()));
				std::ignore = p1.add(make_random_string<CharType>(length));
				expect(p1.size() == 2_ull) << fatal;

				pool_type p2{};
				std::ignore = p2.add(make_random_string<CharType>(length));
				std::ignore = p2.add(make_random_string<CharType>(p2.block_initial_size()));
				expect(p2.size() == 2_ull) << fatal;

				pool_type p3{};
				std::ignore = p3.add(make_random_string<CharType>(length));
				std::ignore = p3.add(make_random_string<CharType>(length));
				expect((length + IsNullTerminate) * 2 < value(pool_type::default_block_initial_size)) << fatal;
				expect(p3.size() == 1_ull) << fatal;

				pool_type p{std::move(p1), p2, std::move(p3)};
				expect(p1.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p2.size() == 2_ull) << fatal;
				expect(p3.size() == 0_ull) << fatal; // NOLINT(bugprone-use-after-move)
				expect(p.size() == 5_ull) << fatal;
			};
		};

		const auto test_block_size = []<typename CharType, bool IsNullTerminate>() noexcept -> void
		{
			WORKAROUND_MSVC_C3688

			using pool_type = StringPool<CharType, IsNullTerminate>;

			expect(200 + IsNullTerminate < value(pool_type::default_block_initial_size));

			pool_type p{100};

			std::ignore = p.add(make_random_string<CharType>(200));
			std::ignore = p.add(make_random_string<CharType>(50));
			// (200 + IsNullTerminate) / (200 + IsNullTerminate)
			// (50 + IsNullTerminate) / (100)
			expect(p.size() == 2_ull) << fatal;

			p.reset_block_initial_size(200);
			std::ignore = p.add(make_random_string<CharType>(200));
			std::ignore = p.add(make_random_string<CharType>(50));
			if constexpr (IsNullTerminate)
			{
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (50 + IsNullTerminate) / (100)
				// (50 + IsNullTerminate) / (200)
				expect(p.size() == 4_ull) << fatal;

				std::ignore = p.add(make_random_string<CharType>(200 - 51 - IsNullTerminate));
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (50 + IsNullTerminate) + (200 - 51 - IsNullTerminate) / 200
				// (50 + IsNullTerminate) / 100
				expect(p.size() == 4_ull) << fatal;

				std::ignore = p.add(make_random_string<CharType>(100 - 51 - IsNullTerminate));
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (200 + IsNullTerminate) / (200 + IsNullTerminate)
				// (50 + IsNullTerminate) + (200 - 51 - IsNullTerminate) / 200
				// (50 + IsNullTerminate) + (100 - 51 - IsNullTerminate) / 100
				expect(p.size() == 4_ull) << fatal;
			}
			else
			{
				// (200) / (200)
				// (200) / (200)
				// (50) + (50) / (100)
				expect(p.size() == 3_ull) << fatal;
			}
		};

		"pool"_test = [test_pool]
		{
			test_pool.operator()<char, false>();
			test_pool.operator()<char8_t, false>();
			test_pool.operator()<char16_t, false>();
			test_pool.operator()<char32_t, false>();
			test_pool.operator()<wchar_t, false>();

			test_pool.operator()<char, true>();
			test_pool.operator()<char8_t, true>();
			test_pool.operator()<char16_t, true>();
			test_pool.operator()<char32_t, true>();
			test_pool.operator()<wchar_t, true>();
		};

		"block_size"_test = [test_block_size]
		{
			test_block_size.operator()<char, false>();
			test_block_size.operator()<char8_t, false>();
			test_block_size.operator()<char16_t, false>();
			test_block_size.operator()<char32_t, false>();
			test_block_size.operator()<wchar_t, false>();

			test_block_size.operator()<char, true>();
			test_block_size.operator()<char8_t, true>();
			test_block_size.operator()<char16_t, true>();
			test_block_size.operator()<char32_t, true>();
			test_block_size.operator()<wchar_t, true>();
		};

		// config().output_level = old_level;
	};
}
