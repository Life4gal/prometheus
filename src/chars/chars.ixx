// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:chars;

import std;

import :platform;

export import :chars.encoding;
export import :chars.scalar;
#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
export import :chars.icelake;
#endif

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <bit>
#include <span>
#include <string>

#include <prometheus/macro.hpp>

#include <platform/platform.ixx>

#include <chars/encoding.ixx>
#include <chars/scalar.ixx>
#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
#include <chars/icelake.ixx>
#endif

#endif

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(chars)
{
	[[nodiscard]] inline auto detect_supported_instruction() noexcept -> std::uint32_t
	{
		const static auto supported = platform::detect_supported_instruction();
		return supported;
	}

	constexpr std::uint32_t icelake_required =
			static_cast<std::uint32_t>(platform::InstructionSet::BMI1) |
			static_cast<std::uint32_t>(platform::InstructionSet::AVX2) |
			static_cast<std::uint32_t>(platform::InstructionSet::BMI2) |
			static_cast<std::uint32_t>(platform::InstructionSet::AVX512BW) |
			static_cast<std::uint32_t>(platform::InstructionSet::AVX512VL) |
			static_cast<std::uint32_t>(platform::InstructionSet::AVX512VBMI2) |
			static_cast<std::uint32_t>(platform::InstructionSet::AVX512VPOPCNTDQ);

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

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: chars
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(chars)
#endif
{
	[[nodiscard]] inline auto encoding_of(const std::span<const char8_t> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			return Encoding<"icelake">::encoding_of(input);
		}
		#else
		#endif

		return Encoding<"scalar">::encoding_of(input);
	}

	[[nodiscard]] inline auto encoding_of(const std::span<const char> input) noexcept -> EncodingType
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
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
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value, ReturnResultType>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<ReturnResultType>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template validate<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value, ReturnResultType>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template validate<ReturnResultType>(input);
		}
	}

	template<CharsCategory InputCategory, bool ReturnResultType = false>
	[[nodiscard]] constexpr auto validate(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> std::conditional_t<ReturnResultType, result_type, bool>
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value, ReturnResultType>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template validate<ReturnResultType>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template validate<GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value, ReturnResultType>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template validate<ReturnResultType>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory>
	[[nodiscard]] constexpr auto length(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> typename scalar_processor_of_t<InputCategory>::size_type
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory>
	[[nodiscard]] constexpr auto length(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> typename scalar_processor_of_t<InputCategory>::size_type
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template length<OutputCategory>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template length<OutputCategory, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
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
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input, output);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input, output);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input, output);
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
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input, output);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input, output);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input, output);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input, output);
		}
	}

	template<CharsCategory InputCategory, typename StringType, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, typename StringType, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> StringType
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<StringType, OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<StringType, OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::input_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input);
		}
	}

	template<CharsCategory InputCategory, CharsCategory OutputCategory, InputProcessPolicy ProcessPolicy = InputProcessPolicy::RETURN_RESULT_TYPE>
	[[nodiscard]] constexpr auto convert(const typename scalar_processor_of_t<InputCategory>::pointer_type input) noexcept -> std::basic_string<typename output_type<OutputCategory>::value_type>
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
			}
			else
			{
				return simd_processor_of_t<InputCategory, "icelake">::template convert<OutputCategory, ProcessPolicy>(input);
			}
		}
		#else
		#endif

		if constexpr (requires { GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value; })
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy, GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::endian_selector<InputCategory>::value>(input);
		}
		else
		{
			return scalar_processor_of_t<InputCategory>::template convert<OutputCategory, ProcessPolicy>(input);
		}
	}

	inline auto flip_endian(const scalar_processor_of_t<CharsCategory::UTF16>::input_type input, const output_type<CharsCategory::UTF16>::pointer output) noexcept -> void
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
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
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			return simd_processor_of_t<CharsCategory::UTF16, "icelake">::flip_endian<StringType>(input);
		}
		#else
		#endif

		return scalar_processor_of_t<CharsCategory::UTF16>::flip_endian<StringType>(input);
	}

	[[nodiscard]] inline auto flip_endian(const scalar_processor_of_t<CharsCategory::UTF16>::input_type input) noexcept -> std::basic_string<output_type<CharsCategory::UTF16>::value_type>
	{
		[[maybe_unused]] const auto supported = GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::detect_supported_instruction();

		#if GAL_PROMETHEUS_CPU_FEATURES_ICELAKE_SUPPORTED
		if ((supported & GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required) == GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::icelake_required)
		{
			return simd_processor_of_t<CharsCategory::UTF16, "icelake">::flip_endian(input);
		}
		#else
		#endif

		return scalar_processor_of_t<CharsCategory::UTF16>::flip_endian(input);
	}
}
