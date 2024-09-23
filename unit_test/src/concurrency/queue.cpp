#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_USE_MODULE
import std;
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"concurrency.queue"> _ = []
	{
		using namespace unit_test;
		using namespace literals;

		constexpr std::size_t producers_count = 1;
		constexpr std::size_t consumers_count = 2;
		constexpr std::size_t queue_capacity = 1024;

		"atomic_queue"_test = []
		{
			using production_type = std::uint32_t;
			using sum_production_type = std::uint64_t;

			constexpr production_type nil_value = std::numeric_limits<production_type>::max();
			constexpr production_type terminate_product = 42;

			constexpr sum_production_type production_per_producer = 1'000'000;
			constexpr auto expected_total_production =
					static_cast<std::uint64_t>(static_cast<double>(production_per_producer + terminate_product + 1) /
					                           2 *
					                           (production_per_producer - terminate_product) *
					                           producers_count
					);

			const auto do_create_consumers = [](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers
			) noexcept -> void
			{
				std::ranges::for_each(
					std::views::zip(sums, consumers),
					[&queue](const std::tuple<std::uint64_t&, std::thread&>& pack) noexcept -> void
					{
						auto& [sum, consumer] = pack;

						consumer = std::thread{
								[&queue, &sum]() noexcept -> void
								{
									std::uint64_t total = 0;
									while (true)
									{
										const auto current = queue.pop();
										if (current == terminate_product)
										{
											break;
										}

										total += current;
									}

									sum = total;
								}
						};
					}
				);
			};

			const auto do_create_producers = [](
				auto& queue,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(
					producers,
					[&queue](std::thread& producer) noexcept -> void
					{
						producer = std::thread{
								[&queue]() noexcept -> void
								{
									for (auto n = production_per_producer; n != terminate_product; --n)
									{
										queue.push(n);
									}
								}
						};
					}
				);
			};

			const auto do_start_and_check = [value = expected_total_production](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(producers, &std::thread::join);
				std::ranges::for_each(std::views::iota(0) | std::views::take(consumers_count), [&queue](auto) { queue.push(terminate_product); });
				std::ranges::for_each(consumers, &std::thread::join);

				const auto total = std::ranges::fold_left(
					sums,
					std::uint64_t{0},
					[](const auto t, const auto c) noexcept -> std::uint64_t { return t + c; }
				);

				using namespace unit_test::operators;
				// using operators::operator==;
				expect(total == unit_test::value(value));
			};

			"fixed_atomic_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = concurrency::FixedAtomicQueue<production_type, queue_capacity, nil_value>;
				queue_type queue{};

				std::uint64_t sums[consumers_count];
				std::thread consumers[consumers_count];
				std::thread producers[producers_count];

				do_create_consumers(queue, sums, consumers);
				do_create_producers(queue, producers);
				do_start_and_check(queue, sums, consumers, producers);
			};

			"dynamic_atomic_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = concurrency::DynamicAtomicQueue<production_type, nil_value>;
				queue_type queue{queue_capacity};

				std::uint64_t sums[consumers_count];
				std::thread consumers[consumers_count];
				std::thread producers[producers_count];

				do_create_consumers(queue, sums, consumers);
				do_create_producers(queue, producers);
				do_start_and_check(queue, sums, consumers, producers);
			};
		};

		"queue"_test = []
		{
			struct production_type
			{
				std::uint32_t hash;
				std::uint32_t uuid_1;
				std::uint32_t uuid_2;
				std::uint64_t uuid_3;
				std::uint32_t uuid_4;
				std::uint32_t id;

				[[nodiscard]] constexpr auto operator==(const production_type& other) const noexcept -> bool = default;
			};
			using sum_production_type = std::uint64_t;

			// fixme
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			#define WORKAROUND_CONSTEXPR const
			#define WORKAROUND_REF_TERMINATE_PRODUCT &terminate_product
			#define WORKAROUND_REF_TERMINATE_PRODUCT_COMMA WORKAROUND_REF_TERMINATE_PRODUCT,
			#else
			#define WORKAROUND_CONSTEXPR constexpr
			#define WORKAROUND_REF_TERMINATE_PRODUCT
			#define WORKAROUND_REF_TERMINATE_PRODUCT_COMMA
			#endif

			constexpr std::uint32_t terminate_product_id = 42;
			// ReSharper disable once CppVariableCanBeMadeConstexpr
			WORKAROUND_CONSTEXPR production_type terminate_product{.hash = 0, .uuid_1 = 0, .uuid_2 = 0, .uuid_3 = 0, .uuid_4 = 0, .id = terminate_product_id};

			constexpr sum_production_type production_per_producer = 1'000'000;
			constexpr auto expected_total_production =
					static_cast<std::uint64_t>(static_cast<double>(production_per_producer + terminate_product_id + 1) /
					                           2 *
					                           (production_per_producer - terminate_product_id) *
					                           producers_count
					);

			const auto do_create_consumers = [WORKAROUND_REF_TERMINATE_PRODUCT](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers
			) noexcept -> void
			{
				std::ranges::for_each(
					std::views::zip(sums, consumers),
					[WORKAROUND_REF_TERMINATE_PRODUCT_COMMA &queue](const std::tuple<std::uint64_t&, std::thread&>& pack) noexcept -> void
					{
						auto& [sum, consumer] = pack;

						consumer = std::thread{
								[WORKAROUND_REF_TERMINATE_PRODUCT_COMMA &queue, &sum]() noexcept -> void
								{
									std::uint64_t total = 0;
									while (true)
									{
										const auto current = queue.pop();
										if (current == terminate_product)
										{
											break;
										}

										total += current.id;
									}

									sum = total;
								}
						};
					}
				);
			};

			const auto do_create_producers = [WORKAROUND_REF_TERMINATE_PRODUCT](
				auto& queue,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(
					producers,
					[WORKAROUND_REF_TERMINATE_PRODUCT_COMMA &queue](std::thread& producer) noexcept -> void
					{
						producer = std::thread{
								[WORKAROUND_REF_TERMINATE_PRODUCT_COMMA &queue]() noexcept -> void
								{
									for (auto n = production_per_producer; n != terminate_product.id; --n)
									{
										queue.push(production_type{
												.hash = static_cast<std::uint32_t>(std::hash<sum_production_type>{}(n)),
												.uuid_1 = 1337,
												.uuid_2 = 1337,
												.uuid_3 = 1337,
												.uuid_4 = 1337,
												.id = static_cast<std::uint32_t>(n)
										});
									}
								}
						};
					}
				);
			};

			const auto do_start_and_check = [WORKAROUND_REF_TERMINATE_PRODUCT_COMMA value = expected_total_production](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(producers, &std::thread::join);
				std::ranges::for_each(std::views::iota(0) | std::views::take(consumers_count), [WORKAROUND_REF_TERMINATE_PRODUCT_COMMA &queue](auto) { queue.push(terminate_product); });
				std::ranges::for_each(consumers, &std::thread::join);

				const auto total = std::ranges::fold_left(
					sums,
					std::uint64_t{0},
					[](const auto t, const auto c) noexcept -> std::uint64_t { return t + c; }
				);

				using namespace unit_test::operators;
				// using operators::operator==;
				expect(total == unit_test::value(value));
			};

			"fixed_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = concurrency::FixedQueue<production_type, queue_capacity>;
				queue_type queue{};

				std::uint64_t sums[consumers_count];
				std::thread consumers[consumers_count];
				std::thread producers[producers_count];

				do_create_consumers(queue, sums, consumers);
				do_create_producers(queue, producers);
				do_start_and_check(queue, sums, consumers, producers);
			};

			"dynamic_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = concurrency::DynamicQueue<production_type>;
				queue_type queue{queue_capacity};

				std::uint64_t sums[consumers_count];
				std::thread consumers[consumers_count];
				std::thread producers[producers_count];

				do_create_consumers(queue, sums, consumers);
				do_create_producers(queue, producers);
				do_start_and_check(queue, sums, consumers, producers);
			};
		};
	};
}
