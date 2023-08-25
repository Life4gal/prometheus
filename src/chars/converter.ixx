// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>
#include <emmintrin.h>

export module gal.prometheus.chars:converter;

import std;
import gal.prometheus.infrastructure;

export namespace gal::prometheus::chars
{
	constexpr std::uint8_t char_placeholder = 0x3f;

	template<typename Derived>
	class CharInvoker
	{
	public:
		using derived_type = Derived;

		using code_point_type = char32_t;
		using chunk_type = __m128i;

	private:
		[[nodiscard]] constexpr auto rep() noexcept -> derived_type& { return *static_cast<derived_type*>(this); }

		[[nodiscard]] constexpr auto rep() const noexcept -> const derived_type& { return *static_cast<const derived_type*>(this); }

	public:
		template<std::input_iterator Iterator>
		[[nodiscard]] constexpr auto endian(const Iterator iterator, const std::size_t size, std::endian e) const noexcept -> std::endian { return rep().do_endian(iterator, size, e); }

		/**
		 * @brief Read a single code-point.
		 * @param begin Pointer to the first code-unit of the code-point to read.
		 * @param end A pointer pointing one beyond the string.
		 * @return [code-point, valid] The function will always return a code-point, even if there was a parse error, in this case `valid` is false.
		 *
		 * @note The begin pointer will point beyond the last code-unit of the code-point that was read.
		 */
		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		[[nodiscard]] constexpr auto read(Begin& begin, const End end) const noexcept -> std::pair<code_point_type, bool>
		{
			GAL_PROMETHEUS_DEBUG_ASSUME(begin != end);

			return rep().do_read(begin, end);
		}

		/**
		 * @brief Encode a single code-point.
		 * @param dest The pointer where the code-units will be written.
		 * @param code_point The code-point to encode.
		 * @return The pointer beyond where the code-units where written.
		 *
		 * @note It is undefined behavior if the ptr does not point to a valid buffer where all the code-units can be written to.
		 */
		// template<std::output_iterator<typename derived_type::value_type> Iterator>
		template<std::input_or_output_iterator Iterator>
		constexpr auto write(Iterator& dest, const code_point_type code_point) const noexcept -> void { return rep().do_write(dest, code_point); }

		/**
		 * @brief Determine number of code-units for a code-point.
		 * @param code_point The code-point to encode.
		 * @return [count, valid] If the code-point can not be encoded `valid` will be false. The count will contain the number of code-unit needed to encode a replacement character.
		 */
		constexpr auto size(const code_point_type code_point) const noexcept -> std::pair<std::uint8_t, bool> { return rep().do_size(code_point); }

		/**
		 * @brief Read a chunk of ASCII characters.
		 * @param source A pointer to the first character of a chunk of 16 characters.
		 * @return 16 bytes in a register, bit 7 must be '1' if the character is not ASCII.
		 *
		 * @note The implementation of this function must set the high-bit of each non-ASCII character.
		 */
		template<std::input_iterator Iterator>
		[[nodiscard]] constexpr auto chunk_read(const Iterator source) const noexcept -> chunk_type
		{
			const auto* p = GAL_PROMETHEUS_START_LIFETIME_AS(chunk_type, std::addressof(*source));
			return _mm_loadu_si128(p);
		}

		/**
		 * @brief Write a chunk of ASCII characters.
		 * @param dest The pointer to the first code-unit where the ASCII characters must be written to.
		 * @param chunk A chunk of 16 ascii characters. bit 7 is always '0'.
		 */
		// template<std::output_iterator<typename derived_type::value_type> Iterator>
		template<std::input_or_output_iterator Iterator>
		constexpr auto chunk_write(const Iterator dest, const chunk_type chunk) const noexcept -> void
		{
			auto* p = GAL_PROMETHEUS_START_LIFETIME_AS(chunk_type, std::addressof(*dest));
			_mm_storeu_si128(p, chunk);
		}

		constexpr auto chunk_size(const chunk_type chunk) const noexcept -> std::size_t
		{
			(void)this;

			const auto mask = _mm_movemask_epi8(chunk);
			if (mask == 0) { return 0; }

			static_assert(sizeof(chunk_type) == 16);
			return std::countr_zero(infrastructure::truncate<std::uint16_t>(mask));
		}
	};

	template<infrastructure::fixed_string Category>
	class CharMap;

	template<infrastructure::fixed_string From, infrastructure::fixed_string To>
	class CharConverter
	{
	public:
		using encoder_from = CharMap<From>;
		using encoder_to = CharMap<To>;

		using value_type_from = typename encoder_from::value_type;
		using value_type_to = typename encoder_to::value_type;

		static_assert(std::is_same_v<typename encoder_from::chunk_type, typename encoder_to::chunk_type>);
		using chunk_type = typename encoder_from::chunk_type;

	private:
		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		[[nodiscard]] constexpr auto do_size(const Begin begin, const End end) const noexcept -> std::pair<std::size_t, bool>
		{
			constexpr auto step = sizeof(chunk_type);

			encoder_from from{};
			encoder_to   to{};

			std::size_t count = 0;
			bool        valid = true;
			for (auto it = begin; ;)
			{
				while (std::ranges::distance(it, end) >= step)
				{
					const auto chunk = from.chunk_read(it);
					if (const auto size = from.chunk_size(chunk);
						size != 0)
					{
						// This chunk contains non-ASCII characters.

						std::ranges::advance(it, size);
						count += size;
						break;
					}

					std::ranges::advance(it, step);
					count += step;
				}

				if (it == end) { break; }

				const auto [code_point, read_valid] = from.read(it, end);
				valid &= read_valid;

				const auto [write_count, write_valid] = to.size(code_point);
				count += write_count;
				valid &= write_valid;
			}

			return {count, valid};
		}

		template<std::ranges::contiguous_range Range>
		[[nodiscard]] constexpr auto do_size(const Range& range) const noexcept -> std::pair<std::size_t, bool> { return do_size(std::ranges::begin(range), std::ranges::end(range)); }

		template<std::input_iterator Begin, std::sentinel_for<Begin> End, std::output_iterator<typename std::iterator_traits<Begin>::value_type> Dest>
		constexpr auto do_convert(const Begin begin, const End end, Dest dest) const noexcept -> void
		{
			constexpr auto step = sizeof(chunk_type);

			encoder_from from{};
			encoder_to   to{};

			for (auto it = begin;;)
			{
				while (std::ranges::distance(it, end) >= step)
				{
					const auto chunk = from.chunk_read(it);
					if (const auto mask = _mm_movemask_epi8(chunk);
						mask != 0)
					{
						// This chunk contains non-ASCII characters.
						break;
					}

					// The complete chunk only contains ASCII characters.
					to.chunk_write(dest, chunk);
					std::ranges::advance(it, step);
					std::ranges::advance(dest, step);
				}

				if (it == end) { break; }

				const auto [code_point, read_valid] = from.read(it, end);
				to.write(dest, code_point);
			}
		}

		template<std::ranges::contiguous_range Range, std::output_iterator<std::ranges::range_value_t<Range>> Dest>
		constexpr auto do_convert(const Range& range, Dest dest) const noexcept -> void { return do_convert(std::ranges::begin(range), std::ranges::end(range), dest); }

	public:
		/**
		 * @brief Convert text between the given encodings.
		 * @param begin An iterator pointing to the first character to be converted.
		 * @param end An iterator pointing one beyond the last character to be converted, or a sentinel.
		 * @return The converted text.
		 */
		template<std::ranges::contiguous_range String = std::basic_string<value_type_to>, std::input_iterator Begin, std::sentinel_for<Begin> End>
			requires requires(String& string) { string.resize(std::declval<typename String::size_type>()); }
		[[nodiscard]] constexpr auto  operator()(const Begin begin, const End end) const noexcept -> String
		{
			const auto [size, valid] = do_size(begin, end);
			if (size == 0) { return String{}; }

			String result{};
			result.resize(static_cast<typename String::size_type>(size));

			if (From == To and valid) { std::ranges::uninitialized_copy(begin, end, std::ranges::begin(result), std::ranges::end(result)); }
			else { do_convert(begin, end, std::ranges::begin(result)); }

			return result;
		}

		/**
		 * @brief Convert text between the given encodings.
		 * @param source The text to be converted.
		 * @return The converted text.
		 */
		template<std::ranges::contiguous_range String = std::basic_string<value_type_to>, std::ranges::contiguous_range In>
			requires requires(String& string) { string.resize(std::declval<typename String::size_type>()); }
		[[nodiscard]] constexpr auto  operator()(In&& source) const noexcept -> String
		{
			const auto [size, valid] = do_size(source);

			if constexpr (From == To and std::is_constructible_v<String, In&&>) { if (valid) { return String{std::forward<In>(source)}; } }

			if (size == 0) { return String{}; }

			String result{};
			result.resize(static_cast<typename String::size_type>(size));

			if (From == To and valid) { std::ranges::uninitialized_copy(source, result); }
			else { do_convert(source, std::ranges::begin(result)); }

			return result;
		}

		/**
		 * @brief Read text from a byte array.
		 * @param iterator An iterator pointing to a byte array containing the text in the `From` encoding.
		 * @param size The number of bytes in the array.
		 * @param e The endianness of characters in the array, used as a hint.
		 * @return The converted text.
		 */
		template<std::ranges::range String = std::basic_string<value_type_to>, std::input_iterator Iterator>
			requires requires(String& string) { string.resize(std::declval<typename String::size_type>()); }
		[[nodiscard]] constexpr auto  operator()(const Iterator iterator, const std::size_t size, std::endian e) const noexcept -> String
		{
			encoder_from from{};
			encoder_to   to{};

			const auto num_chars = size / sizeof(value_type_from);

			const auto real_endian = from.endian(iterator, size, e);
			if (real_endian == std::endian::native and infrastructure::is_floor_align(std::addressof(*iterator))) { return convert<String>(reinterpret_cast<const value_type_from*>(std::addressof(*iterator)), reinterpret_cast<const value_type_from*>(std::addressof(*iterator)) + num_chars); }

			String result{};
			result.resize(static_cast<typename String::size_type>(num_chars));
			std::ranges::uninitialized_copy_n(iterator, num_chars, std::ranges::begin(result), std::ranges::end(result));

			if (real_endian != std::endian::native)
			{
				std::ranges::for_each(
						result,
						[](auto& c) -> void { c = std::byteswap(c); });
			}

			return convert<String>(std::move(result));
		}
	};
}
