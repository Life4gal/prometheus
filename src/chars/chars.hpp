// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <platform/cpu.hpp>

#include <chars/scalar.hpp>
#include<chars/icelake.hpp>

namespace gal::prometheus::chars
{
	namespace chars_detail
	{
		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		constexpr std::uint32_t icelake_required =
				static_cast<std::uint32_t>(platform::InstructionSet::BMI1) |
				static_cast<std::uint32_t>(platform::InstructionSet::AVX2) |
				static_cast<std::uint32_t>(platform::InstructionSet::BMI2) |
				static_cast<std::uint32_t>(platform::InstructionSet::AVX512BW) |
				static_cast<std::uint32_t>(platform::InstructionSet::AVX512VL) |
				static_cast<std::uint32_t>(platform::InstructionSet::AVX512VBMI2) |
				static_cast<std::uint32_t>(platform::InstructionSet::AVX512VPOPCNTDQ);
		#endif
	}

	[[nodiscard]] inline auto encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::encoding_of(input);
		}

		#else

		#endif

		return Scalar::encoding_of(input);
	}

	[[nodiscard]] inline auto encoding_of(const std::span<const char> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::encoding_of(input);
		}

		#else

		#endif

		return Scalar::encoding_of(input);
	}

	template<CharsType InputType>
	[[nodiscard]] constexpr auto validate(const input_type_of<InputType> input) noexcept -> result_error_input_type
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::validate<InputType>(input);
		}

		#else

		#endif

		return Scalar::validate<InputType>(input);
	}

	template<CharsType InputType>
	[[nodiscard]] constexpr auto validate(const typename input_type_of<InputType>::const_pointer input) noexcept -> result_error_input_type
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::validate<InputType>(input);
		}

		#else

		#endif

		return Scalar::validate<InputType>(input);
	}

	template<CharsType InputType, CharsType OutputType>
	[[nodiscard]] constexpr auto length(const input_type_of<InputType> input) noexcept -> typename input_type_of<InputType>::size_type
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::length<InputType, OutputType>(input);
		}

		#else

		#endif

		return Scalar::length<InputType, OutputType>(input);
	}

	template<CharsType InputType, CharsType OutputType>
	[[nodiscard]] constexpr auto length(const typename input_type_of<InputType>::const_pointer input) noexcept -> typename input_type_of<InputType>::size_type
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::length<InputType, OutputType>(input);
		}

		#else

		#endif

		return Scalar::length<InputType, OutputType>(input);
	}

	template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(
		typename output_type_of<OutputType>::pointer output,
		const input_type_of<InputType> input
	) noexcept -> auto
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, Pure, Correct>(output, input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, Pure, Correct>(output, input);
	}

	template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(
		typename output_type_of<OutputType>::pointer output,
		const typename input_type_of<InputType>::const_pointer input
	) noexcept -> auto
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, Pure, Correct>(output, input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, Pure, Correct>(output, input);
	}

	template<CharsType InputType, CharsType OutputType, typename StringType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(const input_type_of<InputType> input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, StringType, Pure, Correct>(input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, StringType, Pure, Correct>(input);
	}

	template<CharsType InputType, CharsType OutputType, typename StringType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(const typename input_type_of<InputType>::const_pointer input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, StringType, Pure, Correct>(input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, StringType, Pure, Correct>(input);
	}

	template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(const input_type_of<InputType> input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, Pure, Correct>(input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, Pure, Correct>(input);
	}

	template<CharsType InputType, CharsType OutputType, bool Pure = false, bool Correct = false>
	[[nodiscard]] constexpr auto convert(const typename input_type_of<InputType>::const_pointer input) noexcept -> std::basic_string<typename output_type_of<OutputType>::value_type>
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::convert<InputType, OutputType, Pure, Correct>(input);
		}

		#else

		#endif

		return Scalar::convert<InputType, OutputType, Pure, Correct>(input);
	}

	inline auto flip(
		const output_type_of<CharsType::UTF16>::pointer output,
		const input_type_of<CharsType::UTF16> input
	) noexcept -> void
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip(output, input);
		}

		#else

		#endif

		return Scalar::flip(output, input);
	}

	inline auto flip(
		const output_type_of<CharsType::UTF16>::pointer output,
		const input_type_of<CharsType::UTF16>::const_pointer input
	) noexcept -> void
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip(output, input);
		}

		#else

		#endif

		return Scalar::flip(output, input);
	}

	template<typename StringType>
	constexpr auto flip(const input_type_of<CharsType::UTF16> input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip<StringType>(input);
		}

		#else

		#endif

		return Scalar::flip<StringType>(input);
	}

	template<typename StringType>
	constexpr auto flip(const input_type_of<CharsType::UTF16>::const_pointer input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip<StringType>(input);
		}

		#else

		#endif

		return Scalar::flip<StringType>(input);
	}

	inline auto flip(const input_type_of<CharsType::UTF16> input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip(input);
		}

		#else

		#endif

		return Scalar::flip(input);
	}

	inline auto flip(const input_type_of<CharsType::UTF16>::const_pointer input) noexcept -> std::basic_string<output_type_of<CharsType::UTF16>::value_type>
	{
		[[maybe_unused]] const auto supported = platform::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED

		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Icelake::flip(input);
		}

		#else

		#endif

		return Scalar::flip(input);
	}
}
