#include <cstdint>

enum class InstructionSet : std::uint32_t
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
extern auto detect_supported_instruction() -> std::uint32_t;

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
