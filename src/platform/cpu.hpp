// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// @see <PROJECT-ROOT>/scripts/detect_supported_instruction.cpp

#pragma once

#include <cstdint>

#include <prometheus/macro.hpp>

namespace gal::prometheus::platform
{
	// ReSharper disable CppInconsistentNaming
	// ReSharper disable IdentifierTypo

	enum class InstructionSet : std::uint32_t  // NOLINT(performance-enum-size)
	{
		DEFAULT = 0b0000'0000'0000'0000,

		PCLMULQDQ = 0b0000'0000'0000'0001,
		SSE42 = 0b0000'0000'0000'0010,
		BMI1 = 0b0000'0000'0000'0100,
		AVX2 = 0b0000'0000'0000'1000,
		BMI2 = 0b0000'0000'0001'0000,
		AVX512F = 0b0000'0000'0010'0000,
		AVX512DQ = 0b0000'0000'0100'0000,
		AVX512CD = 0b0000'0000'1000'0000,
		AVX512BW = 0b0000'0001'0000'0000,
		AVX512VL = 0b0000'0010'0000'0000,
		AVX512VBMI2 = 0b0000'0100'0000'0000,
		AVX512VPOPCNTDQ = 0b0000'1000'0000'0000,
	};

	// ReSharper restore IdentifierTypo
	// ReSharper restore CppInconsistentNaming

	auto detect_supported_instruction() -> std::uint32_t;
}
