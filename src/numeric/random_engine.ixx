// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// A C++ implementation based on [http://prng.di.unimi.it/].

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:numeric.random_engine;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <type_traits>
#include <cstdint>
#include <concepts>
#include <array>
#include <limits>
#include <algorithm>
#include <ranges>
#include <random>

#include <prometheus/macro.hpp>

#endif

#if not defined(CHAR_BIT)
#define CHAR_BIT std::numeric_limits<unsigned char>::digits
#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(numeric)
{
	using bit64_type = std::uint64_t;
	using bit32_type = std::uint32_t;

	template<std::size_t N, std::size_t Total, std::unsigned_integral T>
	constexpr auto rotate_left(const T value) noexcept -> T { return (value << N) | (value >> (Total - N)); }

	template<std::size_t N, std::size_t Total, std::unsigned_integral T>
	constexpr auto rotate_left_to(T& value) noexcept -> void { value = rotate_left<N, Total>(value); }

	template<std::unsigned_integral T, std::size_t StateSize, typename Engine>
	class RandomEngineBase
	{
		[[nodiscard]] auto rep() noexcept -> Engine& { return static_cast<Engine&>(*this); }

		[[nodiscard]] auto rep() const noexcept -> const Engine& { return static_cast<const Engine&>(*this); }

	public:
		using result_type = std::conditional_t<sizeof(T) == sizeof(bit32_type), bit32_type, bit64_type>;
		using state_type = std::array<result_type, StateSize>;

		constexpr static auto bits_of_this = std::numeric_limits<result_type>::digits;

		/**
		 * Output: 64 bits
		 * Period: 2 ^ 64
		 * Footprint: 8 bytes
		 * Original implementation: http://prng.di.unimi.it/splitmix64.c
		 */
		struct state_generator final
		{
			result_type seed;

			[[nodiscard]] constexpr auto state() noexcept -> state_type
			{
				state_type new_state{};
				std::ranges::generate(new_state, *this);
				return new_state;
			}

			[[nodiscard]] constexpr auto operator()() noexcept -> result_type
			{
				seed += static_cast<result_type>(0x9e3779b97f4a7c15ull);

				auto z = seed;
				z = static_cast<result_type>((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull);
				z = static_cast<result_type>((z ^ (z >> 27)) * 0x94d049bb133111ebull);
				return z ^ (z >> 31);
			}
		};

	protected:
		state_type state_;

	private:
		constexpr auto do_jump(const state_type steps) noexcept -> void
		{
			state_type to{};

			std::ranges::for_each(
					steps,
					[this, &to](const auto step) noexcept -> void
					{
						std::ranges::for_each(
								std::views::iota(0, bits_of_this),
								[this, &to, step](const auto mask) noexcept -> void
								{
									if (step & mask) { for (auto& [t, s]: std::views::zip(to, state_)) { t ^= s; } }
									discard();
								},
								[](const auto this_bit) noexcept -> result_type { return result_type{1} << this_bit; });
					});

			state_.swap(to);
		}

	public:
		constexpr explicit RandomEngineBase(const state_type state) noexcept
			: state_{state} {}

		constexpr explicit RandomEngineBase(const result_type seed) noexcept
			: RandomEngineBase{state_generator{.seed = seed}.state()} {}

		constexpr explicit RandomEngineBase() noexcept
			: RandomEngineBase{state_generator{.seed = static_cast<result_type>(std::random_device{}())}.state()} {}

		constexpr RandomEngineBase(const RandomEngineBase&) noexcept = delete;
		constexpr RandomEngineBase(RandomEngineBase&&) noexcept = default;
		constexpr auto operator=(const RandomEngineBase&) noexcept -> RandomEngineBase& = delete;
		constexpr auto operator=(RandomEngineBase&&) noexcept -> RandomEngineBase& = default;

		// todo: MSVC ICE here
		#if not defined(GAL_PROMETHEUS_COMPILER_MSVC)
		constexpr
		#endif
		~RandomEngineBase() noexcept = default;

		[[nodiscard]] constexpr static auto min() noexcept -> result_type { return std::numeric_limits<result_type>::lowest(); }

		[[nodiscard]] constexpr static auto max() noexcept -> result_type { return std::numeric_limits<result_type>::max(); }

		constexpr auto seed(const result_type new_seed) noexcept -> void { *this = RandomEngineBase{new_seed}; }

		[[nodiscard]] constexpr auto peek() const noexcept -> result_type { return rep().do_peek(); }

		constexpr auto next() noexcept -> result_type { return rep().do_next(); }

		constexpr auto discard(const result_type count) noexcept -> void
		{
			// fixme: How to discard values gracefully?
			for (result_type i = 0; i < count; ++i) { next(); }
		}

		[[nodiscard]] constexpr auto operator()() noexcept -> result_type { return next(); }

		constexpr auto jump() noexcept -> void { do_jump(rep().do_jump_state()); }

		constexpr auto long_jump() noexcept -> void { do_jump(rep().do_long_jump_state()); }
	};

	template<std::unsigned_integral T, std::size_t StateSize, typename Engine>
	class Jumper;

	template<typename Engine>
	class Jumper<bit32_type, 4, Engine>
	{
	public:
		using engine_type = RandomEngineBase<bit32_type, 4, Engine>;

		using result_type = typename engine_type::result_type;
		using state_type = typename engine_type::state_type;

		constexpr static auto rotate(state_type& state) noexcept -> void
		{
			const result_type t = state[1] << 9;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= t;
			rotate_left_to<11, engine_type::bits_of_this>(state[3]);
		}

		/**
		* @brief This is the jump function for the generator. It is equivalent
		* to 2 ^ 64 calls to operator(); it can be used to generate 2 ^ 64
		* non-overlapping sub-sequences for parallel computations.
		* @return generated jump steps
		*/
		[[nodiscard]] constexpr static auto jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x8764000bull),
					static_cast<result_type>(0xf542d2d3ull),
					static_cast<result_type>(0x6fa035c3ull),
					static_cast<result_type>(0x77f2db5bull)};
		}

		/**
		* @brief This is the long-jump function for the generator. It is equivalent to
		* 2 ^ 96 calls to operator(); it can be used to generate 2 ^ 32 starting points,
		* from each of which jump() will generate 2 ^ 32 non-overlapping
		* sub-sequences for parallel distributed computations.
		* @return generated long jump steps
		*/
		[[nodiscard]] constexpr static auto long_jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0xb523952eull),
					static_cast<result_type>(0x0b6f099full),
					static_cast<result_type>(0xccf5a0efull),
					static_cast<result_type>(0x1c580662ull)};
		}
	};

	template<typename Engine>
	class Jumper<bit64_type, 2, Engine>
	{
	public:
		using engine_type = RandomEngineBase<bit64_type, 2, Engine>;

		using result_type = typename engine_type::result_type;
		using state_type = typename engine_type::state_type;

		/**
		* @brief This is the jump function for the generator. It is equivalent
		* to 2 ^ 64 calls to next(); it can be used to generate 2 ^ 64
		* non-overlapping sub-sequences for parallel computations.
		* @return generated jump steps
		*/
		[[nodiscard]] constexpr static auto jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0xdf900294d8f554a5ull),
					static_cast<result_type>(0x170865df4b3201fcull)
			};
		}

		/**
		* @brief This is the long-jump function for the generator. It is equivalent to
		* 2 ^ 96 calls to next(); it can be used to generate 2 ^ 32 starting points,
		* from each of which jump() will generate 2 ^ 32 non-overlapping
		* sub-sequences for parallel distributed computations.
		* @return generated long jump steps
		*/
		[[nodiscard]] constexpr static auto long_jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0xd2a98b26625eee7bull),
					static_cast<result_type>(0xdddf9b1090aa7ac1ull)
			};
		}
	};

	template<typename Engine>
	class Jumper<bit64_type, 4, Engine>
	{
	public:
		using engine_type = RandomEngineBase<bit64_type, 4, Engine>;

		using result_type = typename engine_type::result_type;
		using state_type = typename engine_type::state_type;

		constexpr static auto rotate(state_type& state) noexcept -> void
		{
			const result_type t = state[1] << 17;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];

			state[2] ^= t;
			rotate_left_to<45, engine_type::bits_of_this>(state[3]);
		}

		/**
		* @brief This is the jump function for the generator. It is equivalent
		* to 2 ^ 128 calls to operator(); it can be used to generate 2 ^ 128
		* non-overlapping sub-sequences for parallel computations.
		* @return generated jump steps
		*/
		[[nodiscard]] constexpr static auto jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x180ec6d33cfd0abaull),
					static_cast<result_type>(0xd5a61266f0c9392cull),
					static_cast<result_type>(0xa9582618e03fc9aaull),
					static_cast<result_type>(0x39abdc4529b1661cull)
			};
		}

		/**
		* @brief This is the long-jump function for the generator. It is equivalent to
		* 2 ^ 192 calls to operator(); it can be used to generate 2 ^ 64 starting points,
		* from each of which jump() will generate 2 ^ 64 non-overlapping
		* sub-sequences for parallel distributed computations.
		* @return generated long jump steps
		*/
		[[nodiscard]] constexpr static auto long_jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x76e15d3efefdcbbfull),
					static_cast<result_type>(0xc5004e441c522fb3ull),
					static_cast<result_type>(0x77710069854ee241ull),
					static_cast<result_type>(0x39109bb02acbe635ull)
			};
		}
	};

	template<typename Engine>
	class Jumper<bit64_type, 8, Engine>
	{
	public:
		using engine_type = RandomEngineBase<bit64_type, 8, Engine>;

		using result_type = typename engine_type::result_type;
		using state_type = typename engine_type::state_type;

		constexpr static auto rotate(state_type& state) noexcept -> void
		{
			const result_type t = state[1] << 11;

			state[2] ^= state[0];
			state[5] ^= state[1];
			state[1] ^= state[2];
			state[7] ^= state[3];
			state[3] ^= state[4];
			state[4] ^= state[5];
			state[0] ^= state[6];
			state[6] ^= state[7];

			state[6] ^= t;
			rotate_left_to<21, engine_type::bits_of_this>(state[7]);
		}

		/**
		* @brief This is the jump function for the generator. It is equivalent
		* to 2 ^ 256 calls to operator(); it can be used to generate 2 ^ 256
		* non-overlapping sub-sequences for parallel computations.
		* @return generated jump steps
		*/
		[[nodiscard]] constexpr static auto jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x33ed89b6e7a353f9ull),
					static_cast<result_type>(0x760083d7955323beull),
					static_cast<result_type>(0x2837f2fbb5f22faeull),
					static_cast<result_type>(0x4b8c5674d309511cull),
					static_cast<result_type>(0xb11ac47a7ba28c25ull),
					static_cast<result_type>(0xf1be7667092bcc1cull),
					static_cast<result_type>(0x53851efdb6df0aafull),
					static_cast<result_type>(0x1ebbc8b23eaf25dbull)
			};
		}

		/**
		* @brief This is the long-jump function for the generator. It is equivalent to
		* 2 ^ 384 calls to operator(); it can be used to generate 2 ^ 128 starting points,
		* from each of which jump() will generate 2 ^ 128 non-overlapping
		* sub-sequences for parallel distributed computations.
		* @return generated long jump steps
		*/
		[[nodiscard]] constexpr static auto long_jump() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x11467fef8f921d28ull),
					static_cast<result_type>(0xa2a819f2e79c8ea8ull),
					static_cast<result_type>(0xa8299fc284b3959aull),
					static_cast<result_type>(0xb4d347340ca63ee1ull),
					static_cast<result_type>(0x1cb0940bedbff6ceull),
					static_cast<result_type>(0xd956c5c4fa1f8e17ull),
					static_cast<result_type>(0x915e38fd4eda93bcull),
					static_cast<result_type>(0x5b3ccdfa5d7daca5ull)
			};
		}
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(numeric)
{
	enum class RandomEngineCategory
	{
		// xor + shift + rotate
		X_S_R,
		// xor + rotate + shift + rotate
		X_R_S_R,
	};

	enum class RandomEngineTag
	{
		PLUS,
		PLUS_PLUS,
		STAR_STAR,
	};

	enum class RandomEngineBit
	{
		BITS_128,
		BITS_256,
		BITS_512,
	};

	template<RandomEngineCategory Category, RandomEngineTag Tag, RandomEngineBit Bit>
	class RandomEngine;

	/**
	 * @brief Fastest 32-bit generator for 32-bit floating-point numbers.
	 * We suggest to use its upper bits for floating-point generation,
	 * if low linear complexity is not considered an issue (as it is usually
	 * the case) it can be used to generate 32-bit outputs, too.
	 * We suggest to use a sign test to extract a random Boolean value, and
	 * right shifts to extract subsets of bits.
	 * @note
	 * Output: 32 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro128plus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type { return state_[0] + state_[3]; }

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_128_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_128_plus>);
	static_assert(sizeof(random_engine_xsr_128_plus) == 128 / CHAR_BIT);

	/**
	 * @brief 32-bit all-purpose, rock-solid generators.
	 * It has excellent speed, a state size (128 bits) that is large enough for mild parallelism.
	 * @note
	 * Output: 32 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro128plusplus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<7, bits_of_this>(state_[0] + state_[3]) + state_[0];
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_128_plus_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_128_plus_plus>);
	static_assert(sizeof(random_engine_xsr_128_plus_plus) == 128 / CHAR_BIT);

	/**
	 * @brief 32-bit all-purpose, rock-solid generators.
	 * It has excellent speed, a state size (128 bits) that is large enough for mild parallelism.
	 * @note
	 * Output: 32 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro128starstar.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit32_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<7, bits_of_this>(state_[1] * 5) * 9;
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_128_star_star = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_128_star_star>);
	static_assert(sizeof(random_engine_xsr_128_star_star) == 128 / CHAR_BIT);

	/**
	 * @brief Fastest small-state generator for floating-point numbers,
	 * but its state space is large enough only for mild parallelism.
	 * We suggest to use its upper bits for floating-point generation,
	 * if low linear complexity is not considered an issue (as it is usually
	 * the case) it can be used to generate 64-bit outputs, too.
	 * We suggest to use a sign test to extract a random Boolean value, and
	 * right shifts to extract subsets of bits.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoroshiro128plus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				2,
				RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 2, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type { return state_[0] + state_[1]; }

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();

			const auto s1 = state_[1] ^ state_[0];

			state_[0] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<24, bits_of_this>(state_[0]) ^ s1 ^ (s1 << 16);
			state_[1] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<37, bits_of_this>(s1);

			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xrsr_128_plus = RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xrsr_128_plus>);
	static_assert(sizeof(random_engine_xrsr_128_plus) == 128 / CHAR_BIT);

	/**
	 * @brief All-purpose, rock-solid, small-state generators,
	 * its state space is large enough only for mild parallelism.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoroshiro128plusplus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				2,
				RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 2, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<17, bits_of_this>(state_[0] + state_[1]) + state_[0];
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();

			const auto s1 = state_[1] ^ state_[0];

			state_[0] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<49, bits_of_this>(state_[0]) ^ s1 ^ (s1 << 21);
			state_[1] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<28, bits_of_this>(s1);

			return result;
		}

		/**
		 * @brief This is the jump function for the generator. It is equivalent
		 * to 2 ^ 64 calls to operator(); it can be used to generate 2 ^ 64
		 * non-overlapping sub-sequences for parallel computations.
		 * @return generated jump steps
		 */
		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x2bd7a6a6e99c2ddcull),
					static_cast<result_type>(0x0992ccaf6a6fca05ull)
			};
		}

		/**
		 * @brief This is the long-jump function for the generator. It is equivalent to
		 * 2 ^ 96 calls to operator(); it can be used to generate 2 ^ 32 starting points,
		 * from each of which jump() will generate 2 ^ 32 non-overlapping
		 * sub-sequences for parallel distributed computations.
		 * @return generated jump steps
		 */
		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type
		{
			return {
					static_cast<result_type>(0x360fd5f2cf8d5d99ull),
					static_cast<result_type>(0x9c6e6877736c46e3ull)
			};
		}
	};

	using random_engine_xrsr_128_plus_plus = RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xrsr_128_plus_plus>);
	static_assert(sizeof(random_engine_xrsr_128_plus_plus) == 128 / CHAR_BIT);

	/**
	 * @brief All-purpose, rock-solid, small-state generators,
	 * its state space is large enough only for mild parallelism.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 128 - 1 \n
	 * Footprint: 16 bytes \n
	 * @see http://prng.di.unimi.it/xoroshiro128starstar.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				2,
				RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 2, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<7, bits_of_this>(state_[0] * 5) * 9;
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();

			const auto s1 = state_[1] ^ state_[0];

			state_[0] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<24, bits_of_this>(state_[0]) ^ s1 ^ (s1 << 16);
			state_[1] = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<37, bits_of_this>(s1);

			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xrsr_128_star_star = RandomEngine<RandomEngineCategory::X_R_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_128>;

	static_assert(std::uniform_random_bit_generator<random_engine_xrsr_128_star_star>);
	static_assert(sizeof(random_engine_xrsr_128_star_star) == 128 / CHAR_BIT);

	/**
	 * @brief Fastest generator for floating-point numbers.
	 * We suggest to use its upper bits for floating-point generation,
	 * if low linear complexity is not considered an issue (as it is usually the case) it
	 * can be used to generate 64-bit outputs, too.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 256 - 1 \n
	 * Footprint: 32 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro256plus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_256>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_256>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type { return state_[0] + state_[3]; }

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_256_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_256>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_256_plus>);
	static_assert(sizeof(random_engine_xsr_256_plus) == 256 / CHAR_BIT);

	/**
	 * @brief All-purpose, rock-solid generators,
	 * it has excellent (sub-ns) speed, a state (256 bits) that is large
	 * enough for any parallel application.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 256 - 1 \n
	 * Footprint: 32 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro256plusplus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_256>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_256>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<23, bits_of_this>(state_[0] + state_[3]) + state_[0];
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_256_plus_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_256>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_256_plus_plus>);
	static_assert(sizeof(random_engine_xsr_256_plus_plus) == 256 / CHAR_BIT);

	/**
	 * @brief All-purpose, rock-solid generators,
	 * it has excellent (sub-ns) speed, a state (256 bits) that is large
	 * enough for any parallel application.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 256 - 1 \n
	 * Footprint: 32 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro256starstar.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_256>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				4,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_256>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 4, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<7, bits_of_this>(state_[1] * 5) * 9;
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_256_star_star = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_256>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_256_star_star>);
	static_assert(sizeof(random_engine_xsr_256_star_star) == 256 / CHAR_BIT);

	/**
	 * @brief Generator for floating-point numbers with increased state size.
	 * We suggest to use its upper bits for floating-point generation,
	 * if low linear complexity is not considered an issue (as it is usually the case) it
	 * can be used to generate 64-bit outputs, too.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 512 - 1 \n
	 * Footprint: 64 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro512plus.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_512>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				8,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_512>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 8, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type { return state_[0] + state_[3]; }

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_512_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS, RandomEngineBit::BITS_512>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_512_plus>);
	static_assert(sizeof(random_engine_xsr_512_plus) == 512 / CHAR_BIT);

	/**
 * @brief All-purpose, rock-solid generators,
 * it has excellent (sub-ns) speed, a state (512 bits) that is large
 * enough for any parallel application.
 * @note
 * Output: 64 bits \n
 * Period: 2 ^ 512 - 1 \n
 * Footprint: 64 bytes \n
 * @see http://prng.di.unimi.it/xoshiro512plusplus.c
 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_512>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				8,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_512>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 8, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<17, bits_of_this>(state_[0] + state_[2]) + state_[2];
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_512_plus_plus = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::PLUS_PLUS, RandomEngineBit::BITS_512>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_512_plus_plus>);
	static_assert(sizeof(random_engine_xsr_512_plus_plus) == 512 / CHAR_BIT);

	/**
	 * @brief All-purpose, rock-solid generators,
	 * it has excellent (sub-ns) speed, a state (512 bits) that is large
	 * enough for any parallel application.
	 * @note
	 * Output: 64 bits \n
	 * Period: 2 ^ 512 - 1 \n
	 * Footprint: 32 bytes \n
	 * @see http://prng.di.unimi.it/xoshiro512starstar.c
	 */
	template<>
	class RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_512>
			: public
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::RandomEngineBase<
				GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type,
				8,
				RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_512>
			>
	{
		friend RandomEngineBase;

	public:
		using jumper = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::Jumper<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::bit64_type, 8, RandomEngine>;

		using RandomEngineBase::RandomEngineBase;

	private:
		[[nodiscard]] constexpr auto do_peek() const noexcept -> result_type
		{
			return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::rotate_left<7, bits_of_this>(state_[1] * 5) * 9;
		}

		constexpr auto do_next() noexcept -> result_type
		{
			const auto result = do_peek();
			jumper::rotate(state_);
			return result;
		}

		[[nodiscard]] constexpr static auto do_jump_state() noexcept -> state_type { return jumper::jump(); }

		[[nodiscard]] constexpr static auto do_long_jump_state() noexcept -> state_type { return jumper::long_jump(); }
	};

	using random_engine_xsr_512_star_star = RandomEngine<RandomEngineCategory::X_S_R, RandomEngineTag::STAR_STAR, RandomEngineBit::BITS_512>;

	static_assert(std::uniform_random_bit_generator<random_engine_xsr_512_star_star>);
	static_assert(sizeof(random_engine_xsr_512_star_star) == 512 / CHAR_BIT);
}
