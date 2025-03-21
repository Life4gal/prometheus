#include <limits>
#include <string>
#include <thread>
#include <span>
#include <algorithm>

#include <unit_test/unit_test.hpp>
// concurrency::queue
#include <concurrency/concurrency.hpp>

using namespace gal::prometheus;

namespace
{
	GAL_PROMETHEUS_COMPILER_NO_DESTROY unit_test::suite<"concurrency.queue"> _ = []
	{
		using namespace unit_test;
		using namespace concurrency;

		constexpr static std::size_t producers_count = 1;
		constexpr static std::size_t consumers_count = 2;
		constexpr static std::size_t queue_capacity = 1024;

		"atomic_queue"_test = []
		{
			using production_type = std::uint32_t;
			using sum_production_type = std::uint64_t;

			constexpr static production_type nil_value = std::numeric_limits<production_type>::max();
			constexpr static production_type terminate_product = 42;

			constexpr static sum_production_type production_per_producer = 1'000'000;
			[[maybe_unused]] constexpr static auto expected_total_production =
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

			const auto do_start_and_check = [](
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
					[](const auto t, const auto c) noexcept -> std::uint64_t
					{
						return t + c;
					}
				);

				expect(total == value(expected_total_production));
			};

			"fixed_atomic_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = FixedAtomicQueue<production_type, queue_capacity, nil_value>;
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
				using queue_type = DynamicAtomicQueue<production_type, nil_value>;
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
				std::string name;
				std::uint32_t id;

				[[nodiscard]] constexpr auto operator==(const production_type& other) const noexcept -> bool = default;
			};
			using sum_production_type = std::uint64_t;

			constexpr static std::uint32_t terminate_product_id = 42;
			const production_type terminate_product{.name = "", .id = terminate_product_id};

			constexpr static sum_production_type production_per_producer = 1'000'000;
			[[maybe_unused]] constexpr static auto expected_total_production =
					static_cast<std::uint64_t>(static_cast<double>(production_per_producer + terminate_product_id + 1) /
					                           2 *
					                           (production_per_producer - terminate_product_id) *
					                           producers_count
					);

			const auto do_create_consumers = [terminate_product](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers
			) noexcept -> void
			{
				std::ranges::for_each(
					std::views::zip(sums, consumers),
					[&queue, terminate_product](const std::tuple<std::uint64_t&, std::thread&>& pack) noexcept -> void
					{
						auto& [sum, consumer] = pack;

						consumer = std::thread{
								[&queue, &sum, terminate_product]() noexcept -> void
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

			const auto do_create_producers = [terminate_product](
				auto& queue,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(
					producers,
					[&queue, terminate_product](std::thread& producer) noexcept -> void
					{
						producer = std::thread{
								[&queue, terminate_product]() noexcept -> void
								{
									for (auto n = production_per_producer; n != terminate_product.id; --n)
									{
										queue.push(production_type{
												.name = std::format("{}", n),
												.id = static_cast<std::uint32_t>(n)
										});
									}
								}
						};
					}
				);
			};

			const auto do_start_and_check = [terminate_product](
				auto& queue,
				std::span<sum_production_type, consumers_count> sums,
				std::span<std::thread, consumers_count> consumers,
				std::span<std::thread, producers_count> producers
			) noexcept -> void
			{
				std::ranges::for_each(producers, &std::thread::join);
				std::ranges::for_each(std::views::iota(0) | std::views::take(consumers_count), [&queue, terminate_product](auto) { queue.push(terminate_product); });
				std::ranges::for_each(consumers, &std::thread::join);

				const auto total = std::ranges::fold_left(
					sums,
					std::uint64_t{0},
					[](const auto t, const auto c) noexcept -> std::uint64_t
					{
						return t + c;
					}
				);

				expect(total == value(expected_total_production));
			};

			"fixed_queue"_test = [
				do_create_consumers,
				do_create_producers,
				do_start_and_check
			]
			{
				using queue_type = FixedQueue<production_type, queue_capacity>;
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
				using queue_type = DynamicQueue<production_type>;
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
