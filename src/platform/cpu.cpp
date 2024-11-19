// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// @see <PROJECT-ROOT>/scripts/detect_supported_instruction.cpp

#include <cstdint>
#include <bit>
#include <type_traits>

#include <prometheus/macro.hpp>

#include <platform/cpu.hpp>

#if __has_include(<intrin.h>)
#include <intrin.h>
#endif
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#endif

namespace
{
	// @see https://www.felixcloutier.com/x86/cpuid

	// =======================================
	// When CPUID executes with EAX set to 01H, feature information is returned in ECX and EDX.
	// =======================================

	// ReSharper disable CommentTypo
	// ReSharper disable CppInconsistentNaming
	// ReSharper disable IdentifierTypo

	/**
	 * [Figure 3-7](https://www.felixcloutier.com/x86/cpuid#fig-3-7). Feature Information Returned in the ECX Register
	 *
	 * Bit# ----|---- Mnemonic -----|----- Description
	 * 1           | PCLMULQDQ          | PCLMULQDQ. A value of 1 indicates the processor supports the PCLMULQDQ instruction.
	 * 20         | SSE4_2                    | A value of 1 indicates that the processor supports SSE4.2.
	 * 26         | XSAVE                     | A value of 1 indicates that the processor supports the XSAVE/XRSTOR processor extended states feature, the XSETBV/XGETBV instructions, and XCR0.
	 * 27         | OSXSAVE                | A value of 1 indicates that the OS has set CR4.OSXSAVE[bit 18] to enable XSETBV/XGETBV instructions to access XCR0 and to support processor extended state management using XSAVE/XRSTOR.
	 */

	// bit 1 of ECX for EAX=0x01
	constexpr auto pclmulqdq = std::uint32_t{1} << 1;
	// bit 20 of ECX for EAX=0x01
	constexpr auto sse42 = std::uint32_t{1} << 20;
	// bits 26+27 of ECX for EAX=0x01
	constexpr auto osxsave = (std::uint32_t{1} << 26) | (std::uint32_t{1} << 27);

	// =======================================
	// EAX = 0x7H (Structured Extended Feature Flags), ECX = 0x00 (Sub-leaf)
	// =======================================
	/**
	 * EBX:
	 * Bit 00: FSGSBASE. Supports RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE if 1.
	 * Bit 01: IA32_TSC_ADJUST MSR is supported if 1.
	 * Bit 02: SGX. Supports Intel® Software Guard Extensions (Intel® SGX Extensions) if 1.
	 * Bit 03: BMI1.
	 * Bit 04: HLE.
	 * Bit 05: AVX2. Supports Intel® Advanced Vector Extensions 2 (Intel® AVX2) if 1.
	 * Bit 06: FDP_EXCPTN_ONLY. x87 FPU Data Pointer updated only on x87 exceptions if 1.
	 * Bit 07: SMEP. Supports Supervisor-Mode Execution Prevention if 1.
	 * Bit 08: BMI2.
	 * Bit 09: Supports Enhanced REP MOVSB/STOSB if 1.
	 * Bit 10: INVPCID. If 1, supports INVPCID instruction for system software that manages process-context identifiers.
	 * Bit 11: RTM.
	 * Bit 12: RDT-M. Supports Intel® Resource Director Technology (Intel® RDT) Monitoring capability if 1.
	 * Bit 13: Deprecates FPU CS and FPU DS values if 1.
	 * Bit 14: MPX. Supports Intel® Memory Protection Extensions if 1.
	 * Bit 15: RDT-A. Supports Intel® Resource Director Technology (Intel® RDT) Allocation capability if 1.
	 * Bit 16: AVX512F.
	 * Bit 17: AVX512DQ.
	 * Bit 18: RDSEED.
	 * Bit 19: ADX.
	 * Bit 20: SMAP. Supports Supervisor-Mode Access Prevention (and the CLAC/STAC instructions) if 1.
	 * Bit 21: AVX512_IFMA.
	 * Bit 22: Reserved.
	 * Bit 23: CLFLUSHOPT.
	 * Bit 24: CLWB.
	 * Bit 25: Intel Processor Trace.
	 * Bit 26: AVX512PF. (Intel® Xeon PhiTM only.)
	 * Bit 27: AVX512ER. (Intel® Xeon PhiTM only.)
	 * Bit 28: AVX512CD.
	 * Bit 29: SHA. supports Intel® Secure Hash Algorithm Extensions (Intel® SHA Extensions) if 1.
	 * Bit 30: AVX512BW.
	 * Bit 31: AVX512VL.
	 */
	namespace ebx
	{
		constexpr auto bmi1 = std::uint32_t{1} << 3;
		constexpr auto avx2 = std::uint32_t{1} << 5;
		constexpr auto bmi2 = std::uint32_t{1} << 8;
		constexpr auto avx512f = std::uint32_t{1} << 16;
		constexpr auto avx512dq = std::uint32_t{1} << 17;
		// constexpr auto avx512ifma = std::uint32_t{1} << 21;
		constexpr auto avx512cd = std::uint32_t{1} << 28;
		constexpr auto avx512bw = std::uint32_t{1} << 30;
		constexpr auto avx512vl = std::uint32_t{1} << 31;
	} // namespace ebx

	/**
	 * ECX:
	 * Bit 00: PREFETCHWT1. (Intel® Xeon PhiTM only.)
	 * Bit 01: AVX512_VBMI.
	 * Bit 02: UMIP. Supports user-mode instruction prevention if 1.
	 * Bit 03: PKU. Supports protection keys for user-mode pages if 1.
	 * Bit 04: OSPKE. If 1, OS has set CR4.PKE to enable protection keys (and the RDPKRU/WRPKRU instructions).
	 * Bit 05: WAITPKG.
	 * Bit 06: AVX512_VBMI2.
	 * Bit 07: CET_SS. Supports CET shadow stack features if 1. Processors that set this bit define bits 1:0 of the IA32_U_CET and IA32_S_CET MSRs. Enumerates support for the following MSRs: IA32_INTERRUPT_SPP_TABLE_ADDR, IA32_PL3_SSP, IA32_PL2_SSP, IA32_PL1_SSP, and IA32_PL0_SSP.
	 * Bit 08: GFNI.
	 * Bit 09: VAES.
	 * Bit 10: VPCLMULQDQ.
	 * Bit 11: AVX512_VNNI.
	 * Bit 12: AVX512_BITALG.
	 * Bits 13: TME_EN. If 1, the following MSRs are supported: IA32_TME_CAPABILITY, IA32_TME_ACTIVATE, IA32_TME_EXCLUDE_MASK, and IA32_TME_EXCLUDE_BASE.
	 * Bit 14: AVX512_VPOPCNTDQ.
	 * Bit 15: Reserved.
	 * Bit 16: LA57. Supports 57-bit linear addresses and five-level paging if 1.
	 * Bits 21-17: The value of MAWAU used by the BNDLDX and BNDSTX instructions in 64-bit mode.
	 * Bit 22: RDPID and IA32_TSC_AUX are available if 1.
	 * Bit 23: KL. Supports Key Locker if 1.
	 * Bit 24: BUS_LOCK_DETECT. If 1, indicates support for OS bus-lock detection.
	 * Bit 25: CLDEMOTE. Supports cache line demote if 1.
	 * Bit 26: Reserved.
	 * Bit 27: MOVDIRI. Supports MOVDIRI if 1.
	 * Bit 28: MOVDIR64B. Supports MOVDIR64B if 1.
	 * Bit 29: ENQCMD. Supports Enqueue Stores if 1.
	 * Bit 30: SGX_LC. Supports SGX Launch Configuration if 1.
	 * Bit 31: PKS. Supports protection keys for supervisor-mode pages if 1.
	 */
	namespace ecx
	{
		// constexpr auto avx512vbmi      = std::uint32_t{1} << 1;
		constexpr auto avx512vbmi2 = std::uint32_t{1} << 6;
		// constexpr auto avx512vnni      = std::uint32_t{1} << 11;
		// constexpr auto avx512bitalg    = std::uint32_t{1} << 12;
		constexpr auto avx512vpopcntdq = std::uint32_t{1} << 14;
	}

	/**
	 * EDX:
	 * Bit 00: Reserved.
	 * Bit 01: SGX-KEYS. If 1, Attestation Services for Intel® SGX is supported.
	 * Bit 02: AVX512_4VNNIW. (Intel® Xeon PhiTM only.)
	 * Bit 03: AVX512_4FMAPS. (Intel® Xeon PhiTM only.)
	 * Bit 04: Fast Short REP MOV.
	 * Bit 05: UINTR. If 1, the processor supports user interrupts.
	 * Bits 07-06: Reserved. Bit 08: AVX512_VP2INTERSECT.
	 * Bit 09: SRBDS_CTRL. If 1, enumerates support for the IA32_MCU_OPT_CTRL MSR and indicates its bit 0 (RNGDS_MITG_DIS) is also supported.
	 * Bit 10: MD_CLEAR supported.
	 * Bit 11: RTM_ALWAYS_ABORT. If set, any execution of XBEGIN immediately aborts and transitions to the specified fallback address.
	 * Bit 12: Reserved.
	 * Bit 13: If 1, RTM_FORCE_ABORT supported. Processors that set this bit support the IA32_TSX_FORCE_ABORT MSR. They allow software to set IA32_TSX_FORCE_ABORT[0] (RTM_FORCE_ABORT).
	 * Bit 14: SERIALIZE.
	 * Bit 15: Hybrid. If 1, the processor is identified as a hybrid part. If CPUID.0.MAXLEAF ≥ 1AH and CPUID.1A.EAX != 0, then the Native Model ID Enumeration Leaf 1AH exists.
	 * Bit 16: TSXLDTRK. If 1, the processor supports Intel TSX suspend/resume of load address tracking.
	 * Bit 17: Reserved.
	 * Bit 18: PCONFIG. Supports PCONFIG if 1.
	 * Bit 19: Architectural LBRs. If 1, indicates support for architectural LBRs.
	 * Bit 20: CET_IBT. Supports CET indirect branch tracking features if 1. Processors that set this bit define bits 5:2 and bits 63:10 of the IA32_U_CET and IA32_S_CET MSRs.
	 * Bit 21: Reserved.
	 * Bit 22: AMX-BF16. If 1, the processor supports tile computational operations on bfloat16 numbers.
	 * Bit 23: AVX512_FP16.
	 * Bit 24: AMX-TILE. If 1, the processor supports tile architecture.
	 * Bits 25: AMX-INT8. If 1, the processor supports tile computational operations on 8-bit integers.
	 * Bit 26: Enumerates support for indirect branch restricted speculation (IBRS) and the indirect branch predictor barrier (IBPB). Processors that set this bit support the IA32_SPEC_CTRL MSR and the IA32_PRED_CMD MSR. They allow software to set IA32_SPEC_CTRL[0] (IBRS) and IA32_PRED_CMD[0] (IBPB).
	 * Bit 27: Enumerates support for single thread indirect branch predictors (STIBP). Processors that set this bit support the IA32_SPEC_CTRL MSR. They allow software to set IA32_SPEC_CTRL[1] (STIBP).
	 * Bit 28: Enumerates support for L1D_FLUSH. Processors that set this bit support the IA32_FLUSH_CMD MSR. They allow software to set IA32_FLUSH_CMD[0] (L1D_FLUSH).
	 * Bit 29: Enumerates support for the IA32_ARCH_CAPABILITIES MSR.
	 * Bit 30: Enumerates support for the IA32_CORE_CAPABILITIES MSR. IA32_CORE_CAPABILITIES is an architectural MSR that enumerates model-specific features. A bit being set in this MSR indicates that a model specific feature is supported; software must still consult CPUID family/model/stepping to determine the behavior of the enumerated feature as features enumerated in IA32_CORE_CAPABILITIES may have different behavior on different processor models. Some of these features may have behavior that is consistent across processor models (and for which consultation of CPUID family/model/stepping is not necessary); such features are identified explicitly where they are documented in this manual.
	 * Bit 31: Enumerates support for Speculative Store Bypass Disable (SSBD). Processors that set this bit support the IA32_SPEC_CTRL MSR. They allow software to set IA32_SPEC_CTRL[2] (SSBD).
	 */
	namespace edx
	{
		// constexpr auto avx512vp2intersect = std::uint32_t{1} << 8;
	}

	namespace xcr0
	{
		// bit 2 = AVX
		constexpr auto avx256saved = std::uint64_t{0b0100};
		// bits 5,6,7 = opmask, ZMM_hi256, hi16_ZMM
		constexpr auto avx512saved = std::uint64_t{0b1110'0000};
	}

	// ReSharper restore IdentifierTypo
	// ReSharper restore CppInconsistentNaming
	// ReSharper restore CommentTypo

	struct cpu_id_t
	{
		std::uint32_t eax;
		std::uint32_t ebx;
		std::uint32_t ecx;
		std::uint32_t edx;
	};

	auto get_cpu_id(const cpu_id_t config) noexcept -> cpu_id_t
	{
		struct id_t
		{
			int eax;
			int ebx;
			int ecx;
			int edx;
		};
		static_assert(sizeof(id_t) == sizeof(cpu_id_t));
		static_assert(std::is_standard_layout_v<id_t>);

		id_t id{};
		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		__cpuidex(reinterpret_cast<int*>(&id), static_cast<int>(config.eax), static_cast<int>(config.ecx));
		#else
		__asm__ __volatile__(
	        "cpuid"
	        : "=a" (id.eax), "=b" (id.ebx), "=c" (id.ecx), "=d" (id.edx)
	        : "a" (config.eax), "c" (config.ecx)
	    );
		#endif
		return std::bit_cast<cpu_id_t>(id);
	}

	auto get_xcr0() noexcept -> std::uint64_t
	{
		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		return _xgetbv(0);
		#else
		std::uint32_t eax;
		std::uint32_t edx;
		 __asm__ __volatile__ (
	        "xgetbv"
	        : "=a" (eax), "=d" (edx)
	        : "c" (0)
	    );
		return (static_cast<std::uint64_t>(edx) << 32) | eax;
		#endif
	}
}

namespace gal::prometheus::platform
{
	auto detect_supported_instruction() -> std::uint32_t
	{
		std::uint32_t host_isa = 0;

		// EAX = 0x01H
		{
			constexpr cpu_id_t config{.eax = 0x01, .ebx = 0, .ecx = 0, .edx = 0};

			const auto [eax, ebx, ecx, edx] = get_cpu_id(config);

			if (ecx & pclmulqdq) { host_isa |= static_cast<std::uint32_t>(InstructionSet::PCLMULQDQ); }
			if (ecx & sse42) { host_isa |= static_cast<std::uint32_t>(InstructionSet::SSE42); }
			if ((ecx & osxsave) != osxsave) { return host_isa; }
		}

		// checking if the OS saves registers
		const auto xcr0 = get_xcr0();
		if ((xcr0 & xcr0::avx256saved) == 0) { return host_isa; }

		// EAX = 0x07H
		{
			constexpr cpu_id_t config{.eax = 0x07, .ebx = 0, .ecx = 0, .edx = 0};

			const auto [eax, ebx, ecx, edx] = get_cpu_id(config);

			if (ebx & ebx::bmi1) { host_isa |= static_cast<std::uint32_t>(InstructionSet::BMI1); }
			if (ebx & ebx::avx2) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX2); }
			if (ebx & ebx::bmi2) { host_isa |= static_cast<std::uint32_t>(InstructionSet::BMI2); }

			if ((xcr0 & xcr0::avx512saved) != xcr0::avx512saved) { return host_isa; }

			if (ebx & ebx::avx512f) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512F); }
			if (ebx & ebx::avx512dq) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512DQ); }
			if (ebx & ebx::avx512cd) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512CD); }
			if (ebx & ebx::avx512bw) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512BW); }
			if (ebx & ebx::avx512vl) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512VL); }

			if (ecx & ecx::avx512vbmi2) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512VBMI2); }
			if (ecx & ecx::avx512vpopcntdq) { host_isa |= static_cast<std::uint32_t>(InstructionSet::AVX512VPOPCNTDQ); }
		}

		return host_isa;
	}
}
