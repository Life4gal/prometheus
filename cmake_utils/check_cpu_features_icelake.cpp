#include "instruction_set.hpp"

int main()
{
	const auto supported = detect_supported_instruction();

	constexpr std::uint32_t required =
			static_cast<std::uint32_t>(InstructionSet::BMI1) |
			static_cast<std::uint32_t>(InstructionSet::AVX2) |
			static_cast<std::uint32_t>(InstructionSet::BMI2) |
			static_cast<std::uint32_t>(InstructionSet::AVX512BW) |
			static_cast<std::uint32_t>(InstructionSet::AVX512VL) |
			static_cast<std::uint32_t>(InstructionSet::AVX512VBMI2) |
			static_cast<std::uint32_t>(InstructionSet::AVX512VPOPCNTDQ);

	return (supported & required) == required;
}
