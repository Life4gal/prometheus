// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.numeric:random;

import std;
import gal.prometheus.error;

import :random_engine;

namespace gal::prometheus::numeric
{
	template<typename T>
	using default_int_distribution = std::uniform_int_distribution<T>;
	template<typename T>
	using default_floating_point_distribution = std::uniform_real_distribution<T>;
	using default_boolean_distribution = std::bernoulli_distribution;

	namespace random_detail
	{
		struct any {};

		template<template<typename> typename, template<typename> typename>
		struct is_distribution_alias : std::false_type {};

		template<template<typename> typename Target, template<typename> typename Current>
			requires (std::is_same_v<Target<any>, Current<any>>)
		struct is_distribution_alias<Target, Current> : std::true_type {};

		template<template<typename> typename Target, template<typename> typename Current>
		constexpr auto is_distribution_alias_v = is_distribution_alias<Target, Current>::value;

		template<template<typename> typename Distribution, typename T>
		struct is_user_defined_distribution : std::true_type
		{
			// We always assume that the target distribution contains static_assert (or concept) to restrict the type of T. If it does not, then we assume that it supports arbitrary types.
			static_assert(
					std::is_default_constructible_v<Distribution<T>> or
					std::is_constructible_v<Distribution<T>, T> or
					std::is_constructible_v<Distribution<T>, T, T>
					);
		};

		template<template<typename> typename Distribution, typename T>
		constexpr auto is_user_defined_distribution_v = is_user_defined_distribution<Distribution, T>::value;
	}

	export
	{
		template<template<typename> typename, typename>
		struct is_distribution_compatible : std::false_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, short> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, unsigned short> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, int> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, unsigned int> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, long> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, unsigned long> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, long long> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_int_distribution, unsigned long long> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_floating_point_distribution, float> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_floating_point_distribution, double> : std::true_type {};

		template<>
		struct is_distribution_compatible<default_floating_point_distribution, long double> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, short> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, unsigned short> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, int> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, unsigned int> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, long> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, unsigned long> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, long long> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_int_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, unsigned long long> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_floating_point_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, float> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_floating_point_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, double> : std::true_type {};

		template<template<typename> typename DistributionAlias>
			requires (random_detail::is_distribution_alias_v<default_floating_point_distribution, DistributionAlias>)
		struct is_distribution_compatible<DistributionAlias, long double> : std::true_type {};

		// In fact, this holds true for arbitrary types, but if UserDefinedDistribution does not support type T, it should raise a compile error.
		template<template<typename> typename UserDefinedDistribution, typename T>
			requires(random_detail::is_user_defined_distribution_v<UserDefinedDistribution, T>)
		struct is_distribution_compatible<UserDefinedDistribution, T> : std::true_type {};

		template<template<typename> typename Distribution, typename T>
		constexpr bool is_distribution_compatible_v = is_distribution_compatible<Distribution, T>::value;
		template<template<typename> typename Distribution, typename T>
		concept distribution_compatible_t = is_distribution_compatible_v<Distribution, T>;

		enum class RandomStateCategory
		{
			SHARED,
			SHARED_THREAD_ONLY,
			PRIVATE,
		};

		template<
			RandomStateCategory Category,
			typename RandomEngine,
			template<typename>
			typename IntegerDistribution = default_int_distribution,
			template<typename>
			typename FloatingPointDistribution = default_floating_point_distribution,
			typename BooleanDistribution = default_boolean_distribution>
		class Random
		{
		public:
			constexpr static auto category = Category;
			constexpr static auto is_shared_category = category == RandomStateCategory::SHARED or category == RandomStateCategory::SHARED_THREAD_ONLY;

			using engine_type = RandomEngine;

			using result_type = typename engine_type::result_type;

			template<typename T>
			using integer_distribution_type = IntegerDistribution<T>;

			template<typename T>
			using floating_point_distribution_type = FloatingPointDistribution<T>;

			using boolean_distribution_type = BooleanDistribution;

		private:
			[[nodiscard]] constexpr static auto engine() -> engine_type& //
				requires(is_shared_category)
			{
				if constexpr (category == RandomStateCategory::SHARED)
				{
					static engine_type engine{};
					return engine;
				}
				else if (category == RandomStateCategory::SHARED_THREAD_ONLY)
				{
					thread_local engine_type engine{};
					return engine;
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}

			struct engine_local
			{
				engine_type engine{};
			};

			struct engine_empty {};

			using real_engine_type = std::conditional_t<is_shared_category, engine_empty, engine_local>;
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS real_engine_type real_engine_;

			[[nodiscard]] constexpr auto engine() -> engine_type& //
				requires(not is_shared_category) { return real_engine_.engine; }

		public:
			template<typename... Args>
				requires std::is_constructible_v<engine_type, Args...>
			constexpr explicit Random(Args&&... args) noexcept(noexcept(std::is_nothrow_constructible_v<engine_type, Args...>)) //
				requires(not is_shared_category) // PRIVATE ONLY
				: real_engine_{.engine = engine_type{std::forward<Args>(args)...}} {}

			[[nodiscard]] constexpr static auto min() noexcept -> result_type { return engine_type::min(); }

			[[nodiscard]] constexpr static auto max() noexcept -> result_type { return engine_type::max(); }

			constexpr static auto seed(
					const result_type new_seed = static_cast<result_type>(std::chrono::steady_clock::now().time_since_epoch().count()))
				noexcept(noexcept(
					engine().seed(new_seed)
				)) -> void requires(is_shared_category) { engine().seed(new_seed); }

			constexpr auto seed(const result_type new_seed = static_cast<result_type>(std::chrono::steady_clock::now().time_since_epoch().count()))
				noexcept(noexcept(
					std::declval<Random>().engine().seed(new_seed)
				)) -> void requires(not is_shared_category) { engine().seed(new_seed); }

			constexpr static auto discard(const result_type count) noexcept(noexcept(engine().discard(count))) -> void //
				requires(is_shared_category) { engine().discard(count); }

			constexpr static auto discard(const result_type count) noexcept(noexcept(engine().discard(count))) -> void //
				requires(not is_shared_category) { engine().discard(count); }

			/**
			 * @brief Generate a random number in a [from, to] range by *_distribution_type.
			 * @tparam T A distribution compatible type.
			 * @param from The first limit number of a random range.
			 * @param to The second limit number of a random range.
			 * @return A random number in a [from, to] range.
			 */
			template<typename T>
				requires distribution_compatible_t<integer_distribution_type, T> or distribution_compatible_t<floating_point_distribution_type, T>
			constexpr static auto get(
					const T from = std::numeric_limits<T>::min(),
					const std::type_identity_t<T> to = std::numeric_limits<T>::max()
					)
				noexcept(noexcept(
					std::conditional_t<
						distribution_compatible_t<integer_distribution_type, T>,
						integer_distribution_type<T>,
						floating_point_distribution_type<T>
					>{from, static_cast<T>(to)}(engine())
				)) -> T requires(is_shared_category)
			{
				using distribution_type = std::conditional_t<
					distribution_compatible_t<integer_distribution_type, T>,
					integer_distribution_type<T>,
					floating_point_distribution_type<T>
				>;

				if (std::cmp_greater(from, to)) { return distribution_type{static_cast<T>(to), from}(engine()); }
				return distribution_type{from, static_cast<T>(to)}(engine());
			}

			template<typename T>
				requires distribution_compatible_t<integer_distribution_type, T> or distribution_compatible_t<floating_point_distribution_type, T>
			constexpr static auto operator()(
					const T from = std::numeric_limits<T>::min(),
					const std::type_identity_t<T> to = std::numeric_limits<T>::max()
					) noexcept(noexcept(Random::get<T>(from, to))) -> T //
				requires(is_shared_category) { return Random::get<T>(from, to); }

			/**
			 * @brief Generate a random number in a [from, to] range by *_distribution_type.
			 * @tparam T A distribution compatible type.
			 * @param from The first limit number of a random range.
			 * @param to The second limit number of a random range.
			 * @return A random number in a [from, to] range.
			 */
			template<typename T>
				requires distribution_compatible_t<integer_distribution_type, T> or distribution_compatible_t<floating_point_distribution_type, T>
			constexpr auto get(
					const T from = std::numeric_limits<T>::min(),
					const std::type_identity_t<T> to = std::numeric_limits<T>::max()
					)
				noexcept(noexcept(
					std::conditional_t<
						distribution_compatible_t<integer_distribution_type, T>,
						integer_distribution_type<T>,
						floating_point_distribution_type<T>
					>{from, static_cast<T>(to)}(engine())
				)) -> T requires(not is_shared_category)
			{
				using distribution_type = std::conditional_t<
					distribution_compatible_t<integer_distribution_type, T>,
					integer_distribution_type<T>,
					floating_point_distribution_type<T>
				>;

				if (std::cmp_greater(from, to)) { return distribution_type{static_cast<T>(to), from}(engine()); }
				return distribution_type{from, static_cast<T>(to)}(engine());
			}

			template<typename T>
				requires distribution_compatible_t<integer_distribution_type, T> or distribution_compatible_t<floating_point_distribution_type, T>
			constexpr auto operator()(
					const T from = std::numeric_limits<T>::min(),
					const std::type_identity_t<T> to = std::numeric_limits<T>::max()
					) noexcept(noexcept(std::declval<Random>().template get<T>(from, to))) -> T //
				requires(not is_shared_category) { return this->template get<T>(from, to); }

			/**
			 * @brief Generate a bool value with specific probability by boolean_distribution_type.
			 * @param probability The probability of generating true in [0, 1] range, 0 means always false, 1 means always true.
			 * @return 'true' with 'probability' probability ('false' otherwise)
			 */
			template<std::same_as<bool>>
			constexpr static auto get(const double probability = .5) noexcept(noexcept(boolean_distribution_type{probability}(engine()))) -> bool //
				requires(is_shared_category)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(probability >= .0 and probability <= 1.);
				return boolean_distribution_type{probability}(engine());
			}

			template<std::same_as<bool>>
			constexpr static auto operator()(const double probability = .5) noexcept(noexcept(Random::get(probability))) -> bool //
				requires(is_shared_category) { return Random::get(probability); }

			/**
			 * @brief Generate a bool value with specific probability by boolean_distribution_type.
			 * @param probability The probability of generating true in [0, 1] range, 0 means always false, 1 means always true.
			 * @return 'true' with 'probability' probability ('false' otherwise)
			 */
			template<std::same_as<bool>>
			constexpr auto get(const double probability = .5) noexcept(noexcept(boolean_distribution_type{probability}(engine()))) -> bool //
				requires(not is_shared_category)
			{
				GAL_PROMETHEUS_DEBUG_ASSUME(probability >= .0 and probability <= 1.);
				return boolean_distribution_type{probability}(engine());
			}

			template<std::same_as<bool>>
			constexpr auto operator()(const double probability = .5) noexcept(noexcept(std::declval<Random>().get(probability))) -> bool //
				requires(not is_shared_category) { return this->get(probability); }

			/**
			 * @brief Return random iterator from iterator range.
			 * @tparam Iterator The type of the iterator.
			 * @param from The start of the range.
			 * @param to The end of the rage.
			 * @return A random iterator from [from, to) range.
			 * @note If from == to, return to.
			 */
			template<typename Iterator>
				requires requires { typename std::iterator_traits<Iterator>::iterator_category; }
			constexpr static auto get(
					Iterator from,
					Iterator to
					)
				noexcept(noexcept(
					std::ranges::next(from, get<typename std::iterator_traits<Iterator>::difference_type>(0, 0))
				)) -> Iterator requires(is_shared_category)
			{
				if (const auto diff = std::ranges::distance(from, to);
					diff == 0) { return to; }
				else { return std::ranges::next(from, Random::get<typename std::iterator_traits<Iterator>::difference_type>(0, diff - 1)); }
			}

			template<typename Iterator>
				requires requires { typename std::iterator_traits<Iterator>::iterator_category; }
			constexpr static auto operator()(Iterator from, Iterator to) noexcept(noexcept(Random::get<Iterator>(from, to))) -> Iterator //
				requires(is_shared_category) { return Random::get<Iterator>(from, to); }

			/**
			 * @brief Return random iterator from iterator range.
			 * @tparam Iterator The type of the iterator.
			 * @param from The start of the range.
			 * @param to The end of the rage.
			 * @return A random iterator from [from, to) range.
			 * @note If from == to, return to.
			 */
			template<typename Iterator>
				requires requires { typename std::iterator_traits<Iterator>::iterator_category; }
			constexpr auto get(
					Iterator from,
					Iterator to
					)
				noexcept(noexcept(
					std::ranges::next(from, get<typename std::iterator_traits<Iterator>::difference_type>(0, 0))
				)) -> Iterator requires(not is_shared_category)
			{
				if (const auto diff = std::ranges::distance(from, to);
					diff == 0) { return to; }
				else { return std::ranges::next(from, this->template get<typename std::iterator_traits<Iterator>::difference_type>(0, diff - 1)); }
			}

			template<typename Iterator>
				requires requires { typename std::iterator_traits<Iterator>::iterator_category; }
			constexpr auto operator()(
					Iterator from,
					Iterator to
					) noexcept(noexcept(std::declval<Random>().template get<Iterator>(from, to))) -> Iterator //
				requires(not is_shared_category) { return this->template get<Iterator>(from, to); }

			/**
			 * @brief Return random iterator from range.
			 * @tparam Range The type of the range.
			 * @param range The range.
			 * @return A random iterator from range.
			 */
			template<std::ranges::range Range>
			constexpr static auto get(Range& range) noexcept(noexcept(get(std::ranges::begin(range), std::ranges::end(range)))) -> auto //
				requires(is_shared_category) { return Random::get(std::ranges::begin(range), std::ranges::end(range)); }

			template<std::ranges::range Range>
			constexpr static auto operator()(Range& range) noexcept(noexcept(Random::get<Range>(range))) -> auto //
				requires(is_shared_category) { return Random::get<Range>(range); }

			/**
			 * @brief Return random iterator from range.
			 * @tparam Range The type of the range.
			 * @param range The range.
			 * @return A random iterator from range.
			 */
			template<std::ranges::range Range>
			constexpr auto get(Range& range) noexcept(noexcept(get(std::ranges::begin(range), std::ranges::end(range)))) -> auto //
				requires(not is_shared_category) { return this->get(std::ranges::begin(range), std::ranges::end(range)); }

			template<std::ranges::range Range>
			constexpr auto operator()(Range& range) noexcept(noexcept(std::declval<Random>().template get<Range>(range))) -> auto //
				requires(not is_shared_category) { return this->template get<Range>(range); }

			/**
			 * @brief Fill a container with random values.
			 * @tparam Container The type of container.
			 * @tparam T The type of random value.
			 * @param container The container.
			 * @param from The first limit number of a random range.
			 * @param to The second limit number of a random range.
			 * @param count The number of elements in resulting container.
			 * @note The user can call `reserve` on the container before passing in the container to avoid multiple memory allocations.
			 * @note The container must be able to accept push_back<T>().
			 */
			template<typename Container, typename T>
			constexpr static auto get(
					Container& container,
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					)
				noexcept(noexcept(
					std::ranges::generate_n(
							std::back_inserter(container),
							count,
							[from, to]() noexcept(noexcept(Random::get(from, to))) { return Random::get(from, to); }) //
				)) -> void requires(is_shared_category)
			{
				std::ranges::generate_n(
						std::back_inserter(container),
						count,
						[from, to]() noexcept(noexcept(Random::get(from, to))) { return Random::get(from, to); });
			}

			template<typename Container, typename T>
			constexpr static auto operator()(
					Container& container,
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					) noexcept(noexcept(Random::get<Container, T>(container, from, to, count))) -> void //
				requires(is_shared_category) { return Random::get<Container, T>(container, from, to, count); }

			/**
			 * @brief Fill a container with random values.
			 * @tparam Container The type of container.
			 * @tparam T The type of random value.
			 * @param container The container.
			 * @param from The first limit number of a random range.
			 * @param to The second limit number of a random range.
			 * @param count The number of elements in resulting container.
			 * @note The user can call `reserve` on the container before passing in the container to avoid multiple memory allocations.
			 * @note The container must be able to accept push_back<T>().
			 */
			template<typename Container, typename T>
			constexpr auto get(
					Container& container,
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count)
				noexcept(noexcept(
					std::ranges::generate_n(
							std::back_inserter(container),
							count,
							[from, to]() noexcept(noexcept(std::declval<Random>().get(from, to))) { return Random::get(from, to); }) //
				)) -> void requires(not is_shared_category)
			{
				std::ranges::generate_n(
						std::back_inserter(container),
						count,
						[from, to, this]() noexcept(noexcept(this->get(from, to))) { return this->get(from, to); });
			}

			template<typename Container, typename T>
			constexpr auto operator()(
					Container& container,
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					)
				noexcept(noexcept(
					std::declval<Random>().template get<Container, T>(
							container,
							from,
							to,
							count)
				)) -> void requires(not is_shared_category) { return this->template get<Container, T>(container, from, to, count); }

			template<typename Container, typename T>
			constexpr static auto get(
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					)
				noexcept(noexcept(
					Random::get(std::declval<Container&>(), from, to, count)
				)) -> Container requires(is_shared_category)
			{
				Container c{};

				if constexpr (requires { c.reserve(count); }) { c.reserve(count); }

				Random::get(c, from, to, count);
				return c;
			}

			template<typename Container, typename T>
			constexpr static auto operator()(
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					) noexcept(noexcept(Random::get<Container, T>(from, to, count))) -> Container //
				requires(is_shared_category) { return Random::get<Container, T>(from, to, count); }

			template<typename Container, typename T>
			constexpr auto get(
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					)
				noexcept(noexcept(
					std::declval<Random>().get(std::declval<Container&>(), from, to, count)
				)) -> Container requires(not is_shared_category)
			{
				Container c{};

				if constexpr (requires { c.reserve(count); }) { c.reserve(count); }

				this->get(c, from, to, count);
				return c;
			}

			template<typename Container, typename T>
			constexpr auto operator()(
					const T from,
					const std::type_identity_t<T> to,
					const std::size_t count
					) noexcept(noexcept(std::declval<Random>().template get<Container, T>(from, to, count))) -> Container //
				requires(not is_shared_category) { return this->template get<Container, T>(from, to, count); }

			template<typename Distribution, typename... Args>
			constexpr static auto get(Args&&... args)
				noexcept(noexcept(Distribution{std::forward<Args>(args)...}(engine()))) -> typename Distribution::result_type //
				requires(is_shared_category) { return Distribution{std::forward<Args>(args)...}(engine()); }

			template<typename Distribution, typename... Args>
			constexpr static auto operator()(Args&&... args)
				noexcept(noexcept(Random::get<Distribution, Args...>(std::forward<Args>(args)...))) -> typename Distribution::result_type //
				requires(is_shared_category) { return Random::get<Distribution, Args...>(std::forward<Args>(args)...); }

			template<typename Distribution, typename... Args>
			constexpr auto get(Args&&... args)
				noexcept(noexcept(Distribution{std::forward<Args>(args)...}(std::declval<Random>().engine()))) -> typename Distribution::result_type
				//
				requires(not is_shared_category) { return Distribution{std::forward<Args>(args)...}(engine()); }

			template<typename Distribution, typename... Args>
			constexpr auto operator()(Args&&... args)
				noexcept(noexcept(
					std::declval<Random>().template get<Distribution, Args...>(std::forward<Args>(args)...)
				)) -> typename Distribution::result_type //
				requires(not is_shared_category) { return this->template get<Distribution, Args...>(std::forward<Args>(args)...); }
		};
	}
}
