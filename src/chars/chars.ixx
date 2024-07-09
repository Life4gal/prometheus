// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.chars;

import gal.prometheus.error;

export import :encoding;

import :scalar;

#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
import :icelake;
#endif

#else
#include <prometheus/macro.hpp>
#include <error/error.ixx>
#include <chars/encoding.ixx>
#include <chars/scalar.ixx>

#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
#include <chars/icelake.ixx>
#endif

#endif

namespace gal::prometheus::chars
{
	namespace chars_detail
	{
		[[nodiscard]] inline auto detect_supported_instruction() noexcept -> std::uint32_t
		{
			const static auto supported = error::detect_supported_instruction();
			return supported;
		}

		constexpr std::uint32_t icelake_required =
				static_cast<std::uint32_t>(error::InstructionSet::BMI1) |
				static_cast<std::uint32_t>(error::InstructionSet::AVX2) |
				static_cast<std::uint32_t>(error::InstructionSet::BMI2) |
				static_cast<std::uint32_t>(error::InstructionSet::AVX512BW) |
				static_cast<std::uint32_t>(error::InstructionSet::AVX512VL) |
				static_cast<std::uint32_t>(error::InstructionSet::AVX512VBMI2) |
				static_cast<std::uint32_t>(error::InstructionSet::AVX512VPOPCNTDQ);

		template<CharsCategory InputCategory>
		struct endian_selector;

		template<>
		struct endian_selector<CharsCategory::UTF16>
		{
			constexpr static auto value = std::endian::native;
		};

		template<>
		struct endian_selector<CharsCategory::UTF16_LE>
		{
			constexpr static auto value = std::endian::little;
		};

		template<>
		struct endian_selector<CharsCategory::UTF16_BE>
		{
			constexpr static auto value = std::endian::big;
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	[[nodiscard]] inline auto encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Encoding<"icelake">::encoding_of(input);
		}
		#else
		#endif

		return Encoding<"scalar">::encoding_of(input);
	}

	[[nodiscard]] inline auto encoding_of(const std::span<const char> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return Encoding<"icelake">::encoding_of(input);
		}
		#else
		#endif

		return Encoding<"scalar">::encoding_of(input);
	}

	template<CharsCategory InputCategory, bool ReturnResultType = false>
	[[nodiscard]] constexpr auto validate(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<chars_detail::endian_selector<InputCategory>::value, ReturnResultType>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<ReturnResultType>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template validate<chars_detail::endian_selector<InputCategory>::value, ReturnResultType>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template validate<ReturnResultType>(input);
		}
	}

	template<CharsCategory InputCategory, bool ReturnResultType = false>
	[[nodiscard]] constexpr auto validate(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<chars_detail::endian_selector<InputCategory>::value, ReturnResultType>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<ReturnResultType>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template validate<chars_detail::endian_selector<InputCategory>::value, ReturnResultType>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template validate<ReturnResultType>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory>
	[[nodiscard]] constexpr auto length(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> typename scalar_processor_of_t<InputCategory>::size_type
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory>
	[[nodiscard]] constexpr auto length(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> typename scalar_processor_of_t<InputCategory>::size_type
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(
		const typename scalar_processor_of_t<InputCategory>::input_type input,
		typename output_type<OutputCategory>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input, output);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input, output);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input, output);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input, output);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(
		const typename scalar_processor_of_t<InputCategory>::pointer_type input,
		typename output_type<OutputCategory>::pointer output
	) noexcept -> std::conditional_t<ProcessPolicy == InputProcessPolicy::RETURN_RESULT_TYPE, result_type, std::size_t>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input, output);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input, output);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input, output);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input, output);
		}
	}

	template<CharsCategory InputCategory, typename StringType, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, typename StringType, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { chars_detail::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, chars_detail::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input);
		}
	}

	inline auto flip_endian(const scalar_processor_of_t<CharsCategory::UTF16>::input_type input, const output_type<CharsCategory::UTF16>::pointer output) noexcept -> void
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return simd_processor_of_t<CharsCategory::UTF16, "icelake">::flip_endian(input, output);
		}
		#else
		#endif

		return scalar_processor_of_t<CharsCategory::UTF16>::flip_endian(input, output);
	}

	template<typename StringType>
	[[nodiscard]] constexpr auto flip_endian(const scalar_processor_of_t<CharsCategory::UTF16>::input_type input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return simd_processor_of_t<CharsCategory::UTF16, "icelake">::flip_endian<StringType>(input);
		}
		#else
		#endif

		return scalar_processor_of_t<CharsCategory::UTF16>::flip_endian<StringType>(input);
	}

	[[nodiscard]] inline auto flip_endian(const scalar_processor_of_t<CharsCategory::UTF16>::input_type input) noexcept -> std::basic_string<output_type<CharsCategory::UTF16>::value_type>
	{
		[[maybe_unused]] const auto supported = chars_detail::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & chars_detail::icelake_required) == chars_detail::icelake_required)
		{
			return simd_processor_of_t<CharsCategory::UTF16, "icelake">::flip_endian(input);
		}
		#else
		#endif

		return scalar_processor_of_t<CharsCategory::UTF16>::flip_endian(input);
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
