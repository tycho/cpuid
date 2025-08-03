/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2025, Steven Noonan <steven@uplinklabs.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "prefix.h"

#include "feature.h"
#include "state.h"

#include <stdio.h>
#include <string.h>

/* There are a bunch of base features duplicated in
 * the Extended Features base list on AMD platforms. Define this to decode
 * those as well.
 */
#define SHOW_REDUNDANT

typedef enum
{
	REG_EAX = 0,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_LAST = REG_EDX,
	REG_NULL = 255
} cpu_register_t;

static const char *reg_name(cpu_register_t reg)
{
	static const char *reg_names[] = {
		"eax",
		"ebx",
		"ecx",
		"edx"
	};
	return reg_names[reg];
}

struct cpu_feature_t
{
	uint32_t m_level;
	uint32_t m_index;
	cpu_register_t m_reg;
	uint32_t m_bitmask;
	uint32_t m_vendor;
	const char *m_name;
};

static const struct cpu_feature_t features [] = {
/*  Standard (0000_0001h) */
	{ 0x00000001, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, "x87 FPU on chip"},
	{ 0x00000001, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD, "virtual-8086 mode enhancement"},
	{ 0x00000001, 0, REG_EDX, 0x00000004, VENDOR_INTEL | VENDOR_AMD, "debugging extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000008, VENDOR_INTEL | VENDOR_AMD, "page size extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD, "time stamp counter"},
	{ 0x00000001, 0, REG_EDX, 0x00000020, VENDOR_INTEL | VENDOR_AMD, "RDMSR and WRMSR support"},
	{ 0x00000001, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD, "physical address extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD, "machine check exception"},
	{ 0x00000001, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD, "CMPXCHG8B instruction"},
	{ 0x00000001, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, "APIC on chip"},
/*	{ 0x00000001, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD, "SYSENTER and SYSEXIT instructions"},
	{ 0x00000001, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD, "memory type range registers"},
	{ 0x00000001, 0, REG_EDX, 0x00002000, VENDOR_INTEL | VENDOR_AMD, "PTE global bit"},
	{ 0x00000001, 0, REG_EDX, 0x00004000, VENDOR_INTEL | VENDOR_AMD, "machine check architecture"},
	{ 0x00000001, 0, REG_EDX, 0x00008000, VENDOR_INTEL | VENDOR_AMD, "conditional move instruction"},
	{ 0x00000001, 0, REG_EDX, 0x00010000, VENDOR_INTEL | VENDOR_AMD, "page attribute table"},
	{ 0x00000001, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, "36-bit page size extension"},
	{ 0x00000001, 0, REG_EDX, 0x00040000, VENDOR_INTEL             , "processor serial number"},
	{ 0x00000001, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD, "CLFLUSH instruction"},
/*	{ 0x00000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL             , "debug store"},
	{ 0x00000001, 0, REG_EDX, 0x00400000, VENDOR_INTEL             , "ACPI"},
	{ 0x00000001, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD, "MMX instruction set"},
	{ 0x00000001, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD, "FXSAVE/FXRSTOR instructions"},
	{ 0x00000001, 0, REG_EDX, 0x02000000, VENDOR_INTEL | VENDOR_AMD, "SSE instructions"},
	{ 0x00000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD, "SSE2 instructions"},
	{ 0x00000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL             , "self snoop"},
	{ 0x00000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, "max APIC IDs reserved field is valid"},
	{ 0x00000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL             , "thermal monitor"},
/*	{ 0x00000001, 0, REG_EDX, 0x40000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x80000000, VENDOR_INTEL             , "pending break enable"},

	{ 0x00000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, "SSE3 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000002, VENDOR_INTEL | VENDOR_AMD, "PCLMULQDQ instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00000004, VENDOR_INTEL             , "64-bit DS area"},
	{ 0x00000001, 0, REG_ECX, 0x00000008, VENDOR_INTEL | VENDOR_AMD, "MONITOR/MWAIT instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000010, VENDOR_INTEL             , "CPL qualified debug store"},
	{ 0x00000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL             , "virtual machine extensions"},
	{ 0x00000001, 0, REG_ECX, 0x00000040, VENDOR_INTEL             , "safer mode extensions"},
	{ 0x00000001, 0, REG_ECX, 0x00000080, VENDOR_INTEL             , "Enhanced Intel SpeedStep"},
	{ 0x00000001, 0, REG_ECX, 0x00000100, VENDOR_INTEL             , "thermal monitor 2"},
	{ 0x00000001, 0, REG_ECX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, "SSSE3 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000400, VENDOR_INTEL             , "L1 context ID"},
	{ 0x00000001, 0, REG_ECX, 0x00000800, VENDOR_INTEL             , "silicon debug"}, /* supports IA32_DEBUG_INTERFACE MSR for silicon debug */
	{ 0x00000001, 0, REG_ECX, 0x00001000, VENDOR_INTEL | VENDOR_AMD, "fused multiply-add AVX instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00002000, VENDOR_INTEL | VENDOR_AMD, "CMPXCHG16B instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL             , "xTPR update control"},
	{ 0x00000001, 0, REG_ECX, 0x00008000, VENDOR_INTEL             , "perfmon and debug capability"},
/*	{ 0x00000001, 0, REG_ECX, 0x00010000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, "process-context identifiers"},
	{ 0x00000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL             , "direct cache access"},
	{ 0x00000001, 0, REG_ECX, 0x00080000, VENDOR_INTEL | VENDOR_AMD, "SSE4.1 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, "SSE4.2 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00200000, VENDOR_INTEL | VENDOR_AMD, "x2APIC"},
	{ 0x00000001, 0, REG_ECX, 0x00400000, VENDOR_INTEL | VENDOR_AMD, "MOVBE instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00800000, VENDOR_INTEL | VENDOR_AMD, "POPCNT instruction"},
	{ 0x00000001, 0, REG_ECX, 0x01000000, VENDOR_INTEL | VENDOR_AMD, "TSC deadline"},
	{ 0x00000001, 0, REG_ECX, 0x02000000, VENDOR_INTEL | VENDOR_AMD, "AES instructions"},
	{ 0x00000001, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD, "XSAVE/XRSTOR instructions"},
	{ 0x00000001, 0, REG_ECX, 0x08000000, VENDOR_INTEL | VENDOR_AMD, "OS-enabled XSAVE/XRSTOR"},
	{ 0x00000001, 0, REG_ECX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, "AVX instructions"},
	{ 0x00000001, 0, REG_ECX, 0x20000000, VENDOR_INTEL | VENDOR_AMD, "16-bit FP conversion instructions"},
	{ 0x00000001, 0, REG_ECX, 0x40000000, VENDOR_INTEL | VENDOR_AMD, "RDRAND instruction"},
	{ 0x00000001, 0, REG_ECX, 0x80000000, VENDOR_ANY               , "RAZ (hypervisor)"},

/*  Thermal and Power Management Feature Flags (0000_0006h) */
	{ 0x00000006, 0, REG_EAX, 0x00000001, VENDOR_INTEL             , "Digital temperature sensor"},
	{ 0x00000006, 0, REG_EAX, 0x00000002, VENDOR_INTEL             , "Intel Turbo Boost Technology"},
	{ 0x00000006, 0, REG_EAX, 0x00000004, VENDOR_INTEL | VENDOR_AMD, "Always running APIC timer (ARAT)"},
/*	{ 0x00000006, 0, REG_EAX, 0x00000008, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000006, 0, REG_EAX, 0x00000010, VENDOR_INTEL             , "Power limit notification controls"},
	{ 0x00000006, 0, REG_EAX, 0x00000020, VENDOR_INTEL             , "Clock modulation duty cycle extensions"},
	{ 0x00000006, 0, REG_EAX, 0x00000040, VENDOR_INTEL             , "Package thermal management"},
	{ 0x00000006, 0, REG_EAX, 0x00000080, VENDOR_INTEL             , "Hardware-managed P-state base support (HWP)"},
	{ 0x00000006, 0, REG_EAX, 0x00000100, VENDOR_INTEL             , "HWP notification interrupt enable MSR"},
	{ 0x00000006, 0, REG_EAX, 0x00000200, VENDOR_INTEL             , "HWP activity window MSR"},
	{ 0x00000006, 0, REG_EAX, 0x00000400, VENDOR_INTEL             , "HWP energy/performance preference MSR"},
	{ 0x00000006, 0, REG_EAX, 0x00000800, VENDOR_INTEL             , "HWP package level request MSR"},
/*	{ 0x00000006, 0, REG_EAX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000006, 0, REG_EAX, 0x00002000, VENDOR_INTEL             , "Hardware duty cycle programming (HDC)"},
	{ 0x00000006, 0, REG_EAX, 0x00004000, VENDOR_INTEL             , "Intel Turbo Boost Max Technology 3.0"},
	{ 0x00000006, 0, REG_EAX, 0x00008000, VENDOR_INTEL             , "HWP Capabilities, Highest Performance change"},
	{ 0x00000006, 0, REG_EAX, 0x00010000, VENDOR_INTEL             , "HWP PECI override"},
	{ 0x00000006, 0, REG_EAX, 0x00020000, VENDOR_INTEL             , "Flexible HWP"},
	{ 0x00000006, 0, REG_EAX, 0x00040000, VENDOR_INTEL             , "Fast access mode for IA32_HWP_REQUEST MSR"},
	{ 0x00000006, 0, REG_EAX, 0x00080000, VENDOR_INTEL             , "Hardware feedback MSRs"},
	{ 0x00000006, 0, REG_EAX, 0x00100000, VENDOR_INTEL             , "Ignoring Idle Logical Processor HWP request"},
/*	{ 0x00000006, 0, REG_EAX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000006, 0, REG_EAX, 0x00400000, VENDOR_INTEL             , "HWP control MSR"},
	{ 0x00000006, 0, REG_EAX, 0x00800000, VENDOR_INTEL             , "Enhanced hardware feedback MSRs"},
	{ 0x00000006, 0, REG_EAX, 0x01000000, VENDOR_INTEL             , "Thermal interrupt MSR bit 25"},
/*	{ 0x00000006, 0, REG_EAX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_EAX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_EAX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_EAX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_EAX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_EAX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000006, 0, REG_EAX, 0x80000000, VENDOR_INTEL             , "IP payloads are LIP"},

	{ 0x00000006, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, "Hardware Coordination Feedback Capability (APERF and MPERF)"},
/*	{ 0x00000006, 0, REG_ECX, 0x00000002, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000004, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000006, 0, REG_ECX, 0x00000008, VENDOR_INTEL             , "Performance-energy bias preference"},
/*	{ 0x00000006, 0, REG_ECX, 0x00000010, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000020, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000040, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000080, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000100, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000200, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000400, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00000800, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00002000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00004000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00008000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00010000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00020000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00040000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00080000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00100000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00400000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x00800000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x01000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000006, 0, REG_ECX, 0x80000000, VENDOR_INTEL             , ""}, */   /* Reserved */

/*  Structured Extended Feature Flags (0000_0007h) */
	{ 0x00000007, 0, REG_EBX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, "FSGSBASE instructions"},
	{ 0x00000007, 0, REG_EBX, 0x00000002, VENDOR_INTEL | VENDOR_AMD, "IA32_TSC_ADJUST MSR supported"},
	{ 0x00000007, 0, REG_EBX, 0x00000004, VENDOR_INTEL             , "Software Guard Extensions (SGX)"},
	{ 0x00000007, 0, REG_EBX, 0x00000008, VENDOR_INTEL | VENDOR_AMD, "Bit Manipulation Instructions (BMI1)"},
	{ 0x00000007, 0, REG_EBX, 0x00000010, VENDOR_INTEL             , "Hardware Lock Elision (HLE)"},
	{ 0x00000007, 0, REG_EBX, 0x00000020, VENDOR_INTEL | VENDOR_AMD, "Advanced Vector Extensions 2.0 (AVX2)"},
	{ 0x00000007, 0, REG_EBX, 0x00000040, VENDOR_INTEL             , "x87 FPU data pointer updated only on x87 exceptions"},
	{ 0x00000007, 0, REG_EBX, 0x00000080, VENDOR_INTEL | VENDOR_AMD, "Supervisor Mode Execution Protection (SMEP)"},
	{ 0x00000007, 0, REG_EBX, 0x00000100, VENDOR_INTEL | VENDOR_AMD, "Bit Manipulation Instructions 2 (BMI2)"},
	{ 0x00000007, 0, REG_EBX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, "Enhanced REP MOVSB/STOSB"}, /* Undocumented on AMD */
	{ 0x00000007, 0, REG_EBX, 0x00000400, VENDOR_INTEL | VENDOR_AMD, "INVPCID instruction"}, /* Undocumented on AMD, but instruction documented */
	{ 0x00000007, 0, REG_EBX, 0x00000800, VENDOR_INTEL             , "Restricted Transactional Memory (RTM)"},
	{ 0x00000007, 0, REG_EBX, 0x00001000, VENDOR_INTEL | VENDOR_AMD, "Platform QoS Monitoring (PQM)"},
	{ 0x00000007, 0, REG_EBX, 0x00002000, VENDOR_INTEL             , "x87 FPU CS and DS deprecated"},
	{ 0x00000007, 0, REG_EBX, 0x00004000, VENDOR_INTEL             , "Memory Protection Extensions (MPX)"},
	{ 0x00000007, 0, REG_EBX, 0x00008000, VENDOR_INTEL | VENDOR_AMD, "Platform QoS Enforcement (PQE)"},
	{ 0x00000007, 0, REG_EBX, 0x00010000, VENDOR_INTEL | VENDOR_AMD, "AVX512 foundation (AVX512F)"},
	{ 0x00000007, 0, REG_EBX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, "AVX512 double/quadword instructions (AVX512DQ)"},
	{ 0x00000007, 0, REG_EBX, 0x00040000, VENDOR_INTEL | VENDOR_AMD, "RDSEED instruction"},
	{ 0x00000007, 0, REG_EBX, 0x00080000, VENDOR_INTEL | VENDOR_AMD, "Multi-Precision Add-Carry Instruction Extensions (ADX)"},
	{ 0x00000007, 0, REG_EBX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, "Supervisor Mode Access Prevention (SMAP)"},
	{ 0x00000007, 0, REG_EBX, 0x00200000, VENDOR_INTEL | VENDOR_AMD, "AVX512 integer FMA instructions (AVX512IFMA)"},
	{ 0x00000007, 0, REG_EBX, 0x00400000, VENDOR_INTEL             , "Persistent commit instruction (PCOMMIT)"},
	{ 0x00000007, 0, REG_EBX, 0x00400000,                VENDOR_AMD, "RDPID instruction and TSC_AUX MSR support"},
	{ 0x00000007, 0, REG_EBX, 0x00800000, VENDOR_INTEL | VENDOR_AMD, "CLFLUSHOPT instruction"},
	{ 0x00000007, 0, REG_EBX, 0x01000000, VENDOR_INTEL | VENDOR_AMD, "cache line write-back instruction (CLWB)"},
	{ 0x00000007, 0, REG_EBX, 0x02000000, VENDOR_INTEL             , "Intel Processor Trace"},
	{ 0x00000007, 0, REG_EBX, 0x04000000, VENDOR_INTEL             , "AVX512 prefetch instructions (AVX512PF)"},
	{ 0x00000007, 0, REG_EBX, 0x08000000, VENDOR_INTEL             , "AVX512 exponent/reciprocal instructions (AVX512ER)"},
	{ 0x00000007, 0, REG_EBX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, "AVX512 conflict detection instructions (AVX512CD)"},
	{ 0x00000007, 0, REG_EBX, 0x20000000, VENDOR_INTEL | VENDOR_AMD, "SHA-1/SHA-256 instructions"},
	{ 0x00000007, 0, REG_EBX, 0x40000000, VENDOR_INTEL | VENDOR_AMD, "AVX512 byte/word instructions (AVX512BW)"},
	{ 0x00000007, 0, REG_EBX, 0x80000000, VENDOR_INTEL | VENDOR_AMD, "AVX512 vector length extensions (AVX512VL)"},

	{ 0x00000007, 0, REG_ECX, 0x00000001, VENDOR_INTEL             , "PREFETCHWT1 instruction"},
	{ 0x00000007, 0, REG_ECX, 0x00000002, VENDOR_INTEL             , "AVX512 vector byte manipulation instructions (AVX512VBMI)"},
	{ 0x00000007, 0, REG_ECX, 0x00000004, VENDOR_INTEL | VENDOR_AMD, "User Mode Instruction Prevention (UMIP)"},
	{ 0x00000007, 0, REG_ECX, 0x00000008, VENDOR_INTEL | VENDOR_AMD, "Protection Keys for User-mode pages (PKU)"},
	{ 0x00000007, 0, REG_ECX, 0x00000010, VENDOR_INTEL | VENDOR_AMD, "OS has enabled protection keys (OSPKE)"},
	{ 0x00000007, 0, REG_ECX, 0x00000020, VENDOR_INTEL             , "Wait and Pause Enhancements (WAITPKG)"},
	{ 0x00000007, 0, REG_ECX, 0x00000040, VENDOR_INTEL             , "AVX512_VBMI2"},
	{ 0x00000007, 0, REG_ECX, 0x00000080, VENDOR_INTEL | VENDOR_AMD, "CET shadow stack (CET_SS)"},
	{ 0x00000007, 0, REG_ECX, 0x00000100, VENDOR_INTEL             , "Galois Field NI / Galois Field Affine Transformation (GFNI)"},
	{ 0x00000007, 0, REG_ECX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, "VEX-encoded AES-NI (VAES)"},
	{ 0x00000007, 0, REG_ECX, 0x00000400, VENDOR_INTEL | VENDOR_AMD, "VEX-encoded PCLMUL (VPCL)"},
	{ 0x00000007, 0, REG_ECX, 0x00000800, VENDOR_INTEL             , "AVX512 Vector Neural Network Instructions (AVX512VNNI)"},
	{ 0x00000007, 0, REG_ECX, 0x00001000, VENDOR_INTEL             , "AVX512 Bitwise Algorithms (AVX515BITALG)"},
	{ 0x00000007, 0, REG_ECX, 0x00002000, VENDOR_INTEL             , "Total Memory Encryption (TME_EN)"},
	{ 0x00000007, 0, REG_ECX, 0x00004000, VENDOR_INTEL             , "AVX512 VPOPCNTDQ"},
/*	{ 0x00000007, 0, REG_ECX, 0x00008000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_ECX, 0x00010000, VENDOR_INTEL             , "5-level paging (LA57)"},
/*	{ 0x00000007, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00040000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00080000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00200000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_ECX, 0x00400000, VENDOR_INTEL | VENDOR_AMD, "Read Processor ID (RDPID)"},
	{ 0x00000007, 0, REG_ECX, 0x00800000, VENDOR_INTEL             , "Key locker (KL)"},
	{ 0x00000007, 0, REG_ECX, 0x01000000, VENDOR_INTEL             , "OS bus-lock detection"},
	{ 0x00000007, 0, REG_ECX, 0x02000000, VENDOR_INTEL             , "Cache Line Demote (CLDEMOTE)"},
/*	{ 0x00000007, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_ECX, 0x08000000, VENDOR_INTEL             , "32-bit Direct Stores (MOVDIRI)"},
	{ 0x00000007, 0, REG_ECX, 0x10000000, VENDOR_INTEL             , "64-bit Direct Stores (MOVDIRI64B)"},
	{ 0x00000007, 0, REG_ECX, 0x20000000, VENDOR_INTEL             , "Enqueue Stores (ENQCMD)"},
	{ 0x00000007, 0, REG_ECX, 0x40000000, VENDOR_INTEL             , "SGX Launch Configuration (SGX_LC)"},
	{ 0x00000007, 0, REG_ECX, 0x80000000, VENDOR_INTEL             , "Protection keys for supervisor-mode pages (PKS)"},

/*	{ 0x00000007, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00000004, VENDOR_INTEL             , "AVX512_4VNNIW"},
	{ 0x00000007, 0, REG_EDX, 0x00000008, VENDOR_INTEL             , "AVX512_4FMAPS"},
	{ 0x00000007, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD, "Fast Short REP MOV"}, /* Undocumented on AMD */
	{ 0x00000007, 0, REG_EDX, 0x00000020, VENDOR_INTEL             , "User interrupts (UINTR)"},
/*	{ 0x00000007, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD, "AVX512_VP2INTERSECT"},
/*	{ 0x00000007, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00000400, VENDOR_INTEL             , "MD_CLEAR"},
/*	{ 0x00000007, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00002000, VENDOR_INTEL             , "TSX Force Abort MSR"},
	{ 0x00000007, 0, REG_EDX, 0x00004000, VENDOR_INTEL             , "SERIALIZE"},
	{ 0x00000007, 0, REG_EDX, 0x00008000, VENDOR_INTEL             , "Hybrid"},
	{ 0x00000007, 0, REG_EDX, 0x00010000, VENDOR_INTEL             , "TSX suspend load address tracking"},
/*	{ 0x00000007, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00040000, VENDOR_INTEL             , "PCONFIG"},
	{ 0x00000007, 0, REG_EDX, 0x00080000, VENDOR_INTEL             , "Architectural LBRs"},
	{ 0x00000007, 0, REG_EDX, 0x00100000, VENDOR_INTEL             , "CET indirect branch tracking (CET_IBT)"},
/*	{ 0x00000007, 0, REG_EDX, 0x00200000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EDX, 0x00400000, VENDOR_INTEL             , "Tile computation on bfloat16 (AMX-BF16)"},
	{ 0x00000007, 0, REG_EDX, 0x00800000, VENDOR_INTEL             , "AVX512 FP16"},
	{ 0x00000007, 0, REG_EDX, 0x01000000, VENDOR_INTEL             , "Tile architecture (AMX-TILE)"},
	{ 0x00000007, 0, REG_EDX, 0x02000000, VENDOR_INTEL             , "Tile computation on 8-bit integers (AMX-INT8)"},
	{ 0x00000007, 0, REG_EDX, 0x04000000, VENDOR_INTEL             , "Speculation Control (IBRS and IBPB)"},
	{ 0x00000007, 0, REG_EDX, 0x08000000, VENDOR_INTEL             , "Single Thread Indirect Branch Predictors (STIBP)"},
	{ 0x00000007, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, "L1 Data Cache (L1D) Flush"},
	{ 0x00000007, 0, REG_EDX, 0x20000000, VENDOR_INTEL             , "IA32_ARCH_CAPABILITIES MSR"},
	{ 0x00000007, 0, REG_EDX, 0x40000000, VENDOR_INTEL             , "IA32_CORE_CAPABILITIES MSR"},
	{ 0x00000007, 0, REG_EDX, 0x80000000, VENDOR_INTEL             , "Speculative Store Bypass Disable (SSBD)"},

	{ 0x00000007, 1, REG_EAX, 0x00000001, VENDOR_INTEL             , "SHA512 instructions"},
	{ 0x00000007, 1, REG_EAX, 0x00000002, VENDOR_INTEL             , "SM3 instructions"},
	{ 0x00000007, 1, REG_EAX, 0x00000004, VENDOR_INTEL             , "SM4 instructions"},
/*	{ 0x00000007, 1, REG_EAX, 0x00000008, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x00000010, VENDOR_INTEL | VENDOR_AMD, "AVX Vector Neural Network Instructions (AVX-VNNI)"},
	{ 0x00000007, 1, REG_EAX, 0x00000020, VENDOR_INTEL | VENDOR_AMD, "Vector Neural Network BFLOAT16 (AVX512_BF16)"},
	{ 0x00000007, 1, REG_EAX, 0x00000040, VENDOR_INTEL             , "Linear Address Space Separation"},
	{ 0x00000007, 1, REG_EAX, 0x00000080, VENDOR_INTEL             , "CMPccXADD instruction"},
	{ 0x00000007, 1, REG_EAX, 0x00000100, VENDOR_INTEL             , "Architectural Performance Monitoring Extended leaf valid"},
/*	{ 0x00000007, 1, REG_EAX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x00000400, VENDOR_INTEL             , "Fast zero-length MOVSB"},
	{ 0x00000007, 1, REG_EAX, 0x00000800, VENDOR_INTEL             , "Fast short STOSB"},
	{ 0x00000007, 1, REG_EAX, 0x00001000, VENDOR_INTEL             , "Fast short CMPSB, SCASB"},
/*	{ 0x00000007, 1, REG_EAX, 0x00002000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x00004000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x00008000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x00010000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x00020000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x00040000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x00080000, VENDOR_INTEL             , "WRMSRNS instruction"},
/*	{ 0x00000007, 1, REG_EAX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x00200000, VENDOR_INTEL             , "AMX-FP16 instructions"},
	{ 0x00000007, 1, REG_EAX, 0x00400000, VENDOR_INTEL             , "History reset (HRESET)"},
	{ 0x00000007, 1, REG_EAX, 0x00800000, VENDOR_INTEL             , "AVX-IFMA instructions"},
/*	{ 0x00000007, 1, REG_EAX, 0x01000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x02000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x04000000, VENDOR_INTEL             , "Linear Address Masking (LAM)"},
	{ 0x00000007, 1, REG_EAX, 0x08000000, VENDOR_INTEL             , "RDMSRLIST and WRMSRLIST and IA32_BARRIER MSR"},
/*	{ 0x00000007, 1, REG_EAX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EAX, 0x20000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EAX, 0x40000000, VENDOR_INTEL             , "Supports INVD after BIOS done"},
/*	{ 0x00000007, 1, REG_EAX, 0x80000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */

	{ 0x00000007, 1, REG_EBX, 0x00000001, VENDOR_INTEL             , "IA32_PPIN and IA32_PPIN_CTL"},
/*	{ 0x00000007, 1, REG_EBX, 0x00000002, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000004, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000007, 1, REG_EBX, 0x00000008, VENDOR_INTEL             , "CPUID max val limit removed"},
/*	{ 0x00000007, 1, REG_EBX, 0x00000010, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000020, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000040, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000080, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000100, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000200, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000400, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00000800, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00002000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00004000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00008000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00010000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00020000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00040000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00080000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00100000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00400000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x00800000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x01000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 1, REG_EBX, 0x80000000, VENDOR_INTEL             , ""}, */   /* Reserved */

	{ 0x00000007, 2, REG_EDX, 0x00000001, VENDOR_INTEL             , "Fast store forwarding disable without spec store bypass (PSFD)"},
	{ 0x00000007, 2, REG_EDX, 0x00000002, VENDOR_INTEL             , "IPRED control"},
	{ 0x00000007, 2, REG_EDX, 0x00000004, VENDOR_INTEL             , "RRSBA control"},
	{ 0x00000007, 2, REG_EDX, 0x00000008, VENDOR_INTEL             , "Data dependeng prefetcher control"},
	{ 0x00000007, 2, REG_EDX, 0x00000010, VENDOR_INTEL             , "BHI control"},
	{ 0x00000007, 2, REG_EDX, 0x00000020, VENDOR_INTEL             , "MXCSR Configuration Dependent Timing control"},
	{ 0x00000007, 2, REG_EDX, 0x00000040, VENDOR_INTEL             , "UC-lock disable feature"},
	{ 0x00000007, 2, REG_EDX, 0x00000080, VENDOR_INTEL             , "MONITOR/UMONITOR unaffected by overflow"},
/*	{ 0x00000007, 2, REG_EDX, 0x00000100, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00000200, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00000400, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00000800, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00002000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00004000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00008000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00010000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00020000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00040000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00080000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00100000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00400000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x00800000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x01000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000007, 2, REG_EDX, 0x80000000, VENDOR_INTEL             , ""}, */   /* Reserved */

/*  Processor Trace Enumeration (0000_0014h) */
	{ 0x00000014, 0, REG_EBX, 0x00000001, VENDOR_INTEL             , "CR3 filtering"},
	{ 0x00000014, 0, REG_EBX, 0x00000002, VENDOR_INTEL             , "Configurable PSB, Cycle-Accurate Mode"},
	{ 0x00000014, 0, REG_EBX, 0x00000004, VENDOR_INTEL             , "Filtering preserved across warm reset"},
	{ 0x00000014, 0, REG_EBX, 0x00000008, VENDOR_INTEL             , "MTC timing packet, suppression of COFI-based packets"},
	{ 0x00000014, 0, REG_EBX, 0x00000010, VENDOR_INTEL             , "PTWRITE"},
	{ 0x00000014, 0, REG_EBX, 0x00000020, VENDOR_INTEL             , "Power Event Trace"},
	{ 0x00000014, 0, REG_EBX, 0x00000040, VENDOR_INTEL             , "PSB and PMI preservation MSRs"},
/*	{ 0x00000014, 0, REG_EBX, 0x00000080, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00000100, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00000200, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00000400, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00000800, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00002000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00004000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00008000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00010000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00020000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00040000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00080000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00100000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00400000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x00800000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x01000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_EBX, 0x80000000, VENDOR_INTEL             , ""}, */   /* Reserved */

	{ 0x00000014, 0, REG_ECX, 0x00000001, VENDOR_INTEL             , "ToPA output scheme"},
	{ 0x00000014, 0, REG_ECX, 0x00000002, VENDOR_INTEL             , "ToPA tables hold multiple output entries"},
	{ 0x00000014, 0, REG_ECX, 0x00000004, VENDOR_INTEL             , "Single-range output scheme"},
	{ 0x00000014, 0, REG_ECX, 0x00000008, VENDOR_INTEL             , "Trace Transport output support"},
/*	{ 0x00000014, 0, REG_ECX, 0x00000010, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000020, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000040, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000080, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000100, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000200, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000400, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00000800, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00001000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00002000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00004000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00008000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00010000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00020000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00040000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00080000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00100000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00200000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00400000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x00800000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x01000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x02000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x04000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x08000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x10000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x20000000, VENDOR_INTEL             , ""}, */   /* Reserved */
/*	{ 0x00000014, 0, REG_ECX, 0x40000000, VENDOR_INTEL             , ""}, */   /* Reserved */
	{ 0x00000014, 0, REG_ECX, 0x80000000, VENDOR_INTEL             , "IP payloads are LIP"},

/*  Hypervisor (4000_0001h) */
	{ 0x40000001, 0, REG_EAX, 0x00000001, VENDOR_HV_KVM            , "Clocksource"},
	{ 0x40000001, 0, REG_EAX, 0x00000002, VENDOR_HV_KVM            , "NOP IO Delay"},
	{ 0x40000001, 0, REG_EAX, 0x00000004, VENDOR_HV_KVM            , "MMU Op"},
	{ 0x40000001, 0, REG_EAX, 0x00000008, VENDOR_HV_KVM            , "Clocksource 2"},
	{ 0x40000001, 0, REG_EAX, 0x00000010, VENDOR_HV_KVM            , "Async PF"},
	{ 0x40000001, 0, REG_EAX, 0x00000020, VENDOR_HV_KVM            , "Steal Time"},
	{ 0x40000001, 0, REG_EAX, 0x00000040, VENDOR_HV_KVM            , "PV EOI"},
	{ 0x40000001, 0, REG_EAX, 0x00000080, VENDOR_HV_KVM            , "PV UNHALT"},
/*	{ 0x40000001, 0, REG_EAX, 0x00000100,                          , ""}, */   /* Reserved */
	{ 0x40000001, 0, REG_EAX, 0x00000200, VENDOR_HV_KVM            , "PV TLB flush"},
	{ 0x40000001, 0, REG_EAX, 0x00000400, VENDOR_HV_KVM            , "PV async PF VMEXIT"},
	{ 0x40000001, 0, REG_EAX, 0x00000800, VENDOR_HV_KVM            , "PV send IPI"},
	{ 0x40000001, 0, REG_EAX, 0x00001000, VENDOR_HV_KVM            , "PV poll control"},
	{ 0x40000001, 0, REG_EAX, 0x00002000, VENDOR_HV_KVM            , "PV sched yield"},
	{ 0x40000001, 0, REG_EAX, 0x00004000, VENDOR_HV_KVM            , "Async PF INT"},
	{ 0x40000001, 0, REG_EAX, 0x00008000, VENDOR_HV_KVM            , "MSI extended destination ID"},
	{ 0x40000001, 0, REG_EAX, 0x00010000, VENDOR_HV_KVM            , "Hypercall map GPA range"},
	{ 0x40000001, 0, REG_EAX, 0x00020000, VENDOR_HV_KVM            , "Migration control"},
/*	{ 0x40000001, 0, REG_EAX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
	{ 0x40000001, 0, REG_EAX, 0x01000000, VENDOR_HV_KVM            , "Clocksource Stable"},
/*	{ 0x40000001, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

	{ 0x40000001, 0, REG_EDX, 0x00000001, VENDOR_HV_KVM            , "vCPUs realtime, never preempted"},
/*	{ 0x40000001, 0, REG_EDX, 0x00000002,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000004,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EDX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Hypervisor (4000_0003h) */
	{ 0x40000003, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV         , "VP_RUNTIME"},
	{ 0x40000003, 0, REG_EAX, 0x00000002, VENDOR_HV_HYPERV         , "TIME_REF_COUNT"},
	{ 0x40000003, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV         , "Basic SynIC MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000008, VENDOR_HV_HYPERV         , "Synthetic Timer"},
	{ 0x40000003, 0, REG_EAX, 0x00000010, VENDOR_HV_HYPERV         , "APIC access"},
	{ 0x40000003, 0, REG_EAX, 0x00000020, VENDOR_HV_HYPERV         , "Hypercall MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000040, VENDOR_HV_HYPERV         , "VP Index MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000080, VENDOR_HV_HYPERV         , "System Reset MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000100, VENDOR_HV_HYPERV         , "Access stats MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000200, VENDOR_HV_HYPERV         , "Reference TSC"},
	{ 0x40000003, 0, REG_EAX, 0x00000400, VENDOR_HV_HYPERV         , "Guest Idle MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000800, VENDOR_HV_HYPERV         , "Timer Frequency MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00001000, VENDOR_HV_HYPERV         , "Debug MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00002000, VENDOR_HV_HYPERV         , "Reenlightenment controls"},
/*	{ 0x40000003, 0, REG_EAX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

	{ 0x40000003, 0, REG_EBX, 0x00000001, VENDOR_HV_HYPERV         , "CreatePartitions"},
	{ 0x40000003, 0, REG_EBX, 0x00000002, VENDOR_HV_HYPERV         , "AccessPartitionId"},
	{ 0x40000003, 0, REG_EBX, 0x00000004, VENDOR_HV_HYPERV         , "AccessMemoryPool"},
	{ 0x40000003, 0, REG_EBX, 0x00000008, VENDOR_HV_HYPERV         , "AdjustMemoryBuffers"},
	{ 0x40000003, 0, REG_EBX, 0x00000010, VENDOR_HV_HYPERV         , "PostMessages"},
	{ 0x40000003, 0, REG_EBX, 0x00000020, VENDOR_HV_HYPERV         , "SignalEvents"},
	{ 0x40000003, 0, REG_EBX, 0x00000040, VENDOR_HV_HYPERV         , "CreatePort"},
	{ 0x40000003, 0, REG_EBX, 0x00000080, VENDOR_HV_HYPERV         , "ConnectPort"},
	{ 0x40000003, 0, REG_EBX, 0x00000100, VENDOR_HV_HYPERV         , "AccessStats"},
/*	{ 0x40000003, 0, REG_EBX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x00000400,                          , ""}, */   /* Reserved */
	{ 0x40000003, 0, REG_EBX, 0x00000800, VENDOR_HV_HYPERV         , "Debugging"},
	{ 0x40000003, 0, REG_EBX, 0x00001000, VENDOR_HV_HYPERV         , "CpuManagement"},
	{ 0x40000003, 0, REG_EBX, 0x00002000, VENDOR_HV_HYPERV         , "ConfigureProfiler"},
	{ 0x40000003, 0, REG_EBX, 0x00004000, VENDOR_HV_HYPERV         , "EnableExpandedStackwalking"},
/*	{ 0x40000003, 0, REG_EBX, 0x00008000,                          , ""}, */   /* Reserved */
	{ 0x40000003, 0, REG_EBX, 0x00010000, VENDOR_HV_HYPERV         , "AccessVSM"},
	{ 0x40000003, 0, REG_EBX, 0x00020000, VENDOR_HV_HYPERV         , "AccessVpRegisters"},
/*	{ 0x40000003, 0, REG_EBX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x00080000,                          , ""}, */   /* Reserved */
	{ 0x40000003, 0, REG_EBX, 0x00100000, VENDOR_HV_HYPERV         , "EnableExtendedHypercalls"},
	{ 0x40000003, 0, REG_EBX, 0x00200000, VENDOR_HV_HYPERV         , "StartVirtualProcessor"},
/*	{ 0x40000003, 0, REG_EBX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EBX, 0x80000000,                          , ""}, */   /* Reserved */

	{ 0x40000003, 0, REG_EDX, 0x00000001, VENDOR_HV_HYPERV         , "MWAIT instruction support (deprecated)"},
	{ 0x40000003, 0, REG_EDX, 0x00000002, VENDOR_HV_HYPERV         , "Guest debugging support"},
	{ 0x40000003, 0, REG_EDX, 0x00000004, VENDOR_HV_HYPERV         , "Performance Monitor support"},
	{ 0x40000003, 0, REG_EDX, 0x00000008, VENDOR_HV_HYPERV         , "Physical CPU dynamic partitioning event support"},
	{ 0x40000003, 0, REG_EDX, 0x00000010, VENDOR_HV_HYPERV         , "Hypercall input params via XMM registers"},
	{ 0x40000003, 0, REG_EDX, 0x00000020, VENDOR_HV_HYPERV         , "Virtual guest idle state support"},
	{ 0x40000003, 0, REG_EDX, 0x00000040, VENDOR_HV_HYPERV         , "Hypervisor sleep state support"},
	{ 0x40000003, 0, REG_EDX, 0x00000080, VENDOR_HV_HYPERV         , "NUMA distance query support"},
	{ 0x40000003, 0, REG_EDX, 0x00000100, VENDOR_HV_HYPERV         , "Timer frequency details available"},
	{ 0x40000003, 0, REG_EDX, 0x00000200, VENDOR_HV_HYPERV         , "Synthetic machine check injection support"},
	{ 0x40000003, 0, REG_EDX, 0x00000400, VENDOR_HV_HYPERV         , "Guest crash MSR support"},
	{ 0x40000003, 0, REG_EDX, 0x00000800, VENDOR_HV_HYPERV         , "Debug MSR support"},
	{ 0x40000003, 0, REG_EDX, 0x00001000, VENDOR_HV_HYPERV         , "NPIEP support"},
	{ 0x40000003, 0, REG_EDX, 0x00002000, VENDOR_HV_HYPERV         , "Hypervisor disable support"},
	{ 0x40000003, 0, REG_EDX, 0x00004000, VENDOR_HV_HYPERV         , "Extended GVA ranges for flush virtual address list available"},
	{ 0x40000003, 0, REG_EDX, 0x00008000, VENDOR_HV_HYPERV         , "Hypercall output via XMM registers"},
	{ 0x40000003, 0, REG_EDX, 0x00010000, VENDOR_HV_HYPERV         , "Virtual guest idle state"},
	{ 0x40000003, 0, REG_EDX, 0x00020000, VENDOR_HV_HYPERV         , "Soft interrupt polling mode available"},
	{ 0x40000003, 0, REG_EDX, 0x00040000, VENDOR_HV_HYPERV         , "Hypercall MSR lock available"},
	{ 0x40000003, 0, REG_EDX, 0x00080000, VENDOR_HV_HYPERV         , "Direct synthetic timers support"},
	{ 0x40000003, 0, REG_EDX, 0x00100000, VENDOR_HV_HYPERV         , "PAT register available for VSM"},
	{ 0x40000003, 0, REG_EDX, 0x00200000, VENDOR_HV_HYPERV         , "bndcfgs register available for VSM"},
/*	{ 0x40000003, 0, REG_EDX, 0x00400000,                          , ""}, */   /* Reserved */
	{ 0x40000003, 0, REG_EDX, 0x00800000, VENDOR_HV_HYPERV         , "Synthetic time unhalted timer"},
/*	{ 0x40000003, 0, REG_EDX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x02000000,                          , ""}, */   /* Reserved */
	{ 0x40000003, 0, REG_EDX, 0x04000000, VENDOR_HV_HYPERV         , "Intel Last Branch Record (LBR) feature"},
/*	{ 0x40000003, 0, REG_EDX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Hypervisor implementation recommendations (4000_0004h) */
	{ 0x40000004, 0, REG_EAX, 0x00000001, VENDOR_HV_XEN            , "Virtualized APIC registers"},
	{ 0x40000004, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV         , "Hypercall for address space switches"},
	{ 0x40000004, 0, REG_EAX, 0x00000002, VENDOR_HV_XEN            , "Virtualized x2APIC accesses"},
	{ 0x40000004, 0, REG_EAX, 0x00000002, VENDOR_HV_HYPERV         , "Hypercall for local TLB flushes"},
	{ 0x40000004, 0, REG_EAX, 0x00000004, VENDOR_HV_XEN            , "IOMMU mappings"},
	{ 0x40000004, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV         , "Hypercall for remote TLB flushes"},
	{ 0x40000004, 0, REG_EAX, 0x00000008, VENDOR_HV_XEN            , "VCPU ID present in 40000004:EBX"},
	{ 0x40000004, 0, REG_EAX, 0x00000008, VENDOR_HV_HYPERV         , "MSRs for accessing APIC registers"},
	{ 0x40000004, 0, REG_EAX, 0x00000010, VENDOR_HV_XEN            , "Domain ID present in 40000004:ECX"},
	{ 0x40000004, 0, REG_EAX, 0x00000010, VENDOR_HV_HYPERV         , "Hypervisor MSR for system RESET"},
	{ 0x40000004, 0, REG_EAX, 0x00000020, VENDOR_HV_XEN            , "Extended APIC destination ID"},
	{ 0x40000004, 0, REG_EAX, 0x00000020, VENDOR_HV_HYPERV         , "Relaxed timing"},
	{ 0x40000004, 0, REG_EAX, 0x00000040, VENDOR_HV_XEN            , "Per-vCPU event channel upcalls with PIRQs"},
	{ 0x40000004, 0, REG_EAX, 0x00000040, VENDOR_HV_HYPERV         , "DMA remapping"},
	{ 0x40000004, 0, REG_EAX, 0x00000080, VENDOR_HV_HYPERV         , "Interrupt remapping"},
	{ 0x40000004, 0, REG_EAX, 0x00000100, VENDOR_HV_HYPERV         , "x2APIC MSRs"},
	{ 0x40000004, 0, REG_EAX, 0x00000200, VENDOR_HV_HYPERV         , "Deprecating AutoEOI"},
	{ 0x40000004, 0, REG_EAX, 0x00000400, VENDOR_HV_HYPERV         , "Hypercall for SyntheticClusterIpi"},
	{ 0x40000004, 0, REG_EAX, 0x00000800, VENDOR_HV_HYPERV         , "Interface ExProcessorMasks"},
	{ 0x40000004, 0, REG_EAX, 0x00001000, VENDOR_HV_HYPERV         , "Nested Hyper-V partition"},
	{ 0x40000004, 0, REG_EAX, 0x00002000, VENDOR_HV_HYPERV         , "INT for MBEC system calls"},
	{ 0x40000004, 0, REG_EAX, 0x00004000, VENDOR_HV_HYPERV         , "Enlightenment VMCS interface"},
	{ 0x40000004, 0, REG_EAX, 0x00008000, VENDOR_HV_HYPERV         , "Synced timeline"},
/*	{ 0x40000004, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
	{ 0x40000004, 0, REG_EAX, 0x00020000, VENDOR_HV_HYPERV         , "Direct local flush entire"},
	{ 0x40000004, 0, REG_EAX, 0x00040000, VENDOR_HV_HYPERV         , "No architectural core sharing"},
/*	{ 0x40000004, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000004, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Hypervisor hardware features enabled (4000_0006h) */
	{ 0x40000006, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV         , "APIC overlay assist"},
	{ 0x40000006, 0, REG_EAX, 0x00000002, VENDOR_HV_HYPERV         , "MSR bitmaps"},
	{ 0x40000006, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV         , "Architectural performance counters"},
	{ 0x40000006, 0, REG_EAX, 0x00000008, VENDOR_HV_HYPERV         , "Second-level address translation"},
	{ 0x40000006, 0, REG_EAX, 0x00000010, VENDOR_HV_HYPERV         , "DMA remapping"},
	{ 0x40000006, 0, REG_EAX, 0x00000020, VENDOR_HV_HYPERV         , "Interrupt remapping"},
	{ 0x40000006, 0, REG_EAX, 0x00000040, VENDOR_HV_HYPERV         , "Memory patrol scrubber"},
	{ 0x40000006, 0, REG_EAX, 0x00000080, VENDOR_HV_HYPERV         , "DMA protection"},
	{ 0x40000006, 0, REG_EAX, 0x00000100, VENDOR_HV_HYPERV         , "HPET"},
	{ 0x40000006, 0, REG_EAX, 0x00000200, VENDOR_HV_HYPERV         , "Volatile synthetic timers"},
/*	{ 0x40000006, 0, REG_EAX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x00002000,                          , ""}, */   /* Reserved */
	{ 0x40000006, 0, REG_EAX, 0x00004000, VENDOR_HV_HYPERV         , "Physical destination mode required"},
/*	{ 0x40000006, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
	{ 0x40000006, 0, REG_EAX, 0x00010000, VENDOR_HV_HYPERV         , "Hardware memory zeroing"},
	{ 0x40000006, 0, REG_EAX, 0x00020000, VENDOR_HV_HYPERV         , "Unrestricted guest support"},
	{ 0x40000006, 0, REG_EAX, 0x00040000, VENDOR_HV_HYPERV         , "Resource allocation (RDT-A, PQOS-A)"},
	{ 0x40000006, 0, REG_EAX, 0x00080000, VENDOR_HV_HYPERV         , "Resource monitoring (RDT-M, PQOS-M)"},
	{ 0x40000006, 0, REG_EAX, 0x00100000, VENDOR_HV_HYPERV         , "Guest virtual PMU"},
	{ 0x40000006, 0, REG_EAX, 0x00200000, VENDOR_HV_HYPERV         , "Guest virtual LBR"},
	{ 0x40000006, 0, REG_EAX, 0x00400000, VENDOR_HV_HYPERV         , "Guest virtual IPT"},
	{ 0x40000006, 0, REG_EAX, 0x00800000, VENDOR_HV_HYPERV         , "APIC emulation"},
	{ 0x40000006, 0, REG_EAX, 0x01000000, VENDOR_HV_HYPERV         , "ACPI WDAT table in use"},
/*	{ 0x40000006, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000006, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Hypervisor CPU management features (4000_0007h) */
	{ 0x40000007, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV         , "Start logical processor"},
	{ 0x40000007, 0, REG_EAX, 0x00000002, VENDOR_HV_HYPERV         , "Create root virtual processor"},
	{ 0x40000007, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV         , "Performance counter sync"},
/*	{ 0x40000007, 0, REG_EAX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EAX, 0x80000000,                          , ""}, */   /* ReservedIdentityBit */

	{ 0x40000007, 0, REG_EBX, 0x00000001, VENDOR_HV_HYPERV         , "Processor power management"},
	{ 0x40000007, 0, REG_EBX, 0x00000002, VENDOR_HV_HYPERV         , "MWAIT idle states"},
	{ 0x40000007, 0, REG_EBX, 0x00000004, VENDOR_HV_HYPERV         , "Logical processor idling"},
/*	{ 0x40000007, 0, REG_EBX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_EBX, 0x80000000,                          , ""}, */   /* Reserved */

	{ 0x40000007, 0, REG_ECX, 0x00000001, VENDOR_HV_HYPERV         , "Remap guest uncached"},
/*	{ 0x40000007, 0, REG_ECX, 0x00000002,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000004,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000007, 0, REG_ECX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Hypervisor shared virtual memory (SVM) features (4000_0008h) */
	{ 0x40000008, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV         , "Shared virtual memory (SVM)"},
/*	{ 0x40000008, 0, REG_EAX, 0x00000002,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000004,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000008, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Nested hypervisor feature indentification (4000_0009h) */
/*	{ 0x40000009, 0, REG_EAX, 0x00000001,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00000002,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV         , "Synthetic Timer"},
/*	{ 0x40000009, 0, REG_EAX, 0x00000008,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EAX, 0x00000010, VENDOR_HV_HYPERV         , "Interrupt control registers"},
	{ 0x40000009, 0, REG_EAX, 0x00000020, VENDOR_HV_HYPERV         , "Hypercall MSRs"},
	{ 0x40000009, 0, REG_EAX, 0x00000040, VENDOR_HV_HYPERV         , "VP index MSR"},
/*	{ 0x40000009, 0, REG_EAX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00000800,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EAX, 0x00001000, VENDOR_HV_HYPERV         , "Reenlightenment controls"},
/*	{ 0x40000009, 0, REG_EAX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00020000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */

/*	{ 0x40000009, 0, REG_EDX, 0x00000001,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000002,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000004,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000008,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EDX, 0x00000010, VENDOR_HV_HYPERV         , "Hypercall input params via XMM registers"},
/*	{ 0x40000009, 0, REG_EDX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00004000,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EDX, 0x00008000, VENDOR_HV_HYPERV         , "Hypercall output via XMM registers"},
/*	{ 0x40000009, 0, REG_EDX, 0x00010000,                          , ""}, */   /* Reserved */
	{ 0x40000009, 0, REG_EDX, 0x00020000, VENDOR_HV_HYPERV         , "Soft interrupt polling mode available"},
/*	{ 0x40000009, 0, REG_EDX, 0x00040000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00080000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00100000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x40000009, 0, REG_EDX, 0x80000000,                          , ""}, */   /* Reserved */

/*  Nested hypervisor feature indentification (4000_000Ah) */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000001,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000002,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000004,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000008,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000010,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000020,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000040,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000080,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000100,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000200,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000400,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00000800,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00001000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00002000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00004000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00008000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00010000,                          , ""}, */   /* Reserved */
	{ 0x4000000A, 0, REG_EAX, 0x00020000, VENDOR_HV_HYPERV         , "Direct virtual flush hypercalls"},
	{ 0x4000000A, 0, REG_EAX, 0x00040000, VENDOR_HV_HYPERV         , "Flush GPA space and list hypercalls"},
	{ 0x4000000A, 0, REG_EAX, 0x00080000, VENDOR_HV_HYPERV         , "Enlightened MSR bitmaps"},
	{ 0x4000000A, 0, REG_EAX, 0x00100000, VENDOR_HV_HYPERV         , "Combining virtualization exceptions in page fault exception class"},
/*	{ 0x4000000A, 0, REG_EAX, 0x00200000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00400000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x00800000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x01000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x02000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x04000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x08000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x10000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x20000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x40000000,                          , ""}, */   /* Reserved */
/*	{ 0x4000000A, 0, REG_EAX, 0x80000000,                          , ""}, */   /* Reserved */


/*  Extended (8000_0001h) */
	{ 0x80000001, 0, REG_EDX, 0x00000001,                VENDOR_AMD, "x87 FPU on chip"},
	{ 0x80000001, 0, REG_EDX, 0x00000002,                VENDOR_AMD, "virtual-8086 mode enhancement"},
	{ 0x80000001, 0, REG_EDX, 0x00000004,                VENDOR_AMD, "debugging extensions"},
	{ 0x80000001, 0, REG_EDX, 0x00000008,                VENDOR_AMD, "page size extensions"},
	{ 0x80000001, 0, REG_EDX, 0x00000010,                VENDOR_AMD, "time stamp counter"},
	{ 0x80000001, 0, REG_EDX, 0x00000020,                VENDOR_AMD, "AMD model-specific registers"},
	{ 0x80000001, 0, REG_EDX, 0x00000040,                VENDOR_AMD, "physical address extensions"},
	{ 0x80000001, 0, REG_EDX, 0x00000080,                VENDOR_AMD, "machine check exception"},
	{ 0x80000001, 0, REG_EDX, 0x00000100,                VENDOR_AMD, "CMPXCHG8B instruction"},
	{ 0x80000001, 0, REG_EDX, 0x00000200,                VENDOR_AMD, "APIC on chip"},
/*	{ 0x80000001, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00000800, VENDOR_INTEL             , "SYSENTER and SYSEXIT instructions"},
	{ 0x80000001, 0, REG_EDX, 0x00000800,                VENDOR_AMD, "SYSCALL and SYSRET instructions"},
	{ 0x80000001, 0, REG_EDX, 0x00001000,                VENDOR_AMD, "memory type range registers"},
	{ 0x80000001, 0, REG_EDX, 0x00002000,                VENDOR_AMD, "PTE global bit"},
	{ 0x80000001, 0, REG_EDX, 0x00004000,                VENDOR_AMD, "machine check architecture"},
	{ 0x80000001, 0, REG_EDX, 0x00008000,                VENDOR_AMD, "conditional move instruction"},
	{ 0x80000001, 0, REG_EDX, 0x00010000,                VENDOR_AMD, "page attribute table"},
	{ 0x80000001, 0, REG_EDX, 0x00020000,                VENDOR_AMD, "36-bit page size extension"},
/*	{ 0x80000001, 0, REG_EDX, 0x00040000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL             , "XD bit"},
	{ 0x80000001, 0, REG_EDX, 0x00100000,                VENDOR_AMD, "NX bit"},
/*	{ 0x80000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00400000,                VENDOR_AMD, "MMX extended"},
	{ 0x80000001, 0, REG_EDX, 0x00800000,                VENDOR_AMD, "MMX instructions"},
	{ 0x80000001, 0, REG_EDX, 0x01000000,                VENDOR_AMD, "FXSAVE/FXRSTOR instructions"},
	{ 0x80000001, 0, REG_EDX, 0x02000000,                VENDOR_AMD, "fast FXSAVE/FXRSTOR"},
	{ 0x80000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD, "1GB page support"},
	{ 0x80000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL | VENDOR_AMD, "RDTSCP instruction"},
/*	{ 0x80000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL | VENDOR_AMD, "long mode (EM64T)"},
	{ 0x80000001, 0, REG_EDX, 0x40000000,                VENDOR_AMD, "3DNow! extended"},
	{ 0x80000001, 0, REG_EDX, 0x80000000,                VENDOR_AMD, "3DNow! instructions"},

	{ 0x80000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD, "LAHF/SAHF supported in 64-bit mode"},
	{ 0x80000001, 0, REG_ECX, 0x00000002,                VENDOR_AMD, "core multi-processing legacy mode"},
	{ 0x80000001, 0, REG_ECX, 0x00000004,                VENDOR_AMD, "secure virtual machine (SVM)"},
	{ 0x80000001, 0, REG_ECX, 0x00000008,                VENDOR_AMD, "extended APIC space"},
	{ 0x80000001, 0, REG_ECX, 0x00000010,                VENDOR_AMD, "AltMovCr8"},
	{ 0x80000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL | VENDOR_AMD, "LZCNT instruction"},
	{ 0x80000001, 0, REG_ECX, 0x00000040,                VENDOR_AMD, "SSE4A instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00000080,                VENDOR_AMD, "mis-aligned SSE support"},
	{ 0x80000001, 0, REG_ECX, 0x00000100, VENDOR_INTEL | VENDOR_AMD, "3DNow! prefetch instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00000200,                VENDOR_AMD, "os-visible workaround (OSVW)"},
	{ 0x80000001, 0, REG_ECX, 0x00000400,                VENDOR_AMD, "instruction-based sampling (IBS)"},
	{ 0x80000001, 0, REG_ECX, 0x00000800,                VENDOR_AMD, "extended operation (XOP)"},
	{ 0x80000001, 0, REG_ECX, 0x00001000,                VENDOR_AMD, "SKINIT/STGI instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00002000,                VENDOR_AMD, "watchdog timer (WDT)"},
/*	{ 0x80000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00008000,                VENDOR_AMD, "lightweight profiling (LWP)"},
	{ 0x80000001, 0, REG_ECX, 0x00010000,                VENDOR_AMD, "4-operand FMA instructions (FMA4)"},
	{ 0x80000001, 0, REG_ECX, 0x00020000,                VENDOR_AMD, "Translation cache extension (TCE)"},
/*	{ 0x80000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00080000,                VENDOR_AMD, "node ID support"},
/*	{ 0x80000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00200000,                VENDOR_AMD, "trailing bit manipulation instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00400000,                VENDOR_AMD, "topology extensions"},
	{ 0x80000001, 0, REG_ECX, 0x00800000,                VENDOR_AMD, "processor performance counter extensions"},
	{ 0x80000001, 0, REG_ECX, 0x01000000,                VENDOR_AMD, "NB performance counter extensions"},
	{ 0x80000001, 0, REG_ECX, 0x02000000,                VENDOR_AMD, "streaming performance monitor architecture"},
	{ 0x80000001, 0, REG_ECX, 0x04000000,                VENDOR_AMD, "data access breakpoint extension"},
	{ 0x80000001, 0, REG_ECX, 0x08000000,                VENDOR_AMD, "performance timestamp counter"},
	{ 0x80000001, 0, REG_ECX, 0x10000000,                VENDOR_AMD, "performance counter extensions"},
	{ 0x80000001, 0, REG_ECX, 0x20000000,                VENDOR_AMD, "MONITORX/MWAITX instructions"},
	{ 0x80000001, 0, REG_ECX, 0x40000000,                VENDOR_AMD, "address mask extension for instruction breakpoint"},
/*	{ 0x80000001, 0, REG_ECX, 0x80000000, VENDOR_INTEL | VENDOR_AMD, ""}, */   /* Reserved */

/*  RAS Capabilities (8000_0007h) */
	{ 0x80000007, 0, REG_EBX, 0x00000001,                VENDOR_AMD, "MCA overflow recovery"},
	{ 0x80000007, 0, REG_EBX, 0x00000002,                VENDOR_AMD, "Software uncorrectable error containment and recovery"},
	{ 0x80000007, 0, REG_EBX, 0x00000004,                VENDOR_AMD, "Hardware assert (HWA)"},
	{ 0x80000007, 0, REG_EBX, 0x00000008,                VENDOR_AMD, "Scalable MCA"},
	{ 0x80000007, 0, REG_EBX, 0x00000010,                VENDOR_AMD, "Platform First Error Handling (PFEH)"},

/*  Advanced Power Management information (8000_0007h) */
	{ 0x80000007, 0, REG_EDX, 0x00000001,                VENDOR_AMD, "Temperature Sensor"},
	{ 0x80000007, 0, REG_EDX, 0x00000002,                VENDOR_AMD, "Frequency ID Control"},
	{ 0x80000007, 0, REG_EDX, 0x00000004,                VENDOR_AMD, "Voltage ID Control"},
	{ 0x80000007, 0, REG_EDX, 0x00000008,                VENDOR_AMD, "THERMTRIP"},
	{ 0x80000007, 0, REG_EDX, 0x00000010,                VENDOR_AMD, "Hardware thermal control"},
/*	{ 0x80000007, 0, REG_EDX, 0x00000020,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000007, 0, REG_EDX, 0x00000040,                VENDOR_AMD, "100 MHz multiplier control"},
	{ 0x80000007, 0, REG_EDX, 0x00000080,                VENDOR_AMD, "Hardware P-state control"},
	{ 0x80000007, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD, "Invariant TSC"},
	{ 0x80000007, 0, REG_EDX, 0x00000200,                VENDOR_AMD, "Core performance boost"},
	{ 0x80000007, 0, REG_EDX, 0x00000400,                VENDOR_AMD, "Read-only effective frequency interface"},
	{ 0x80000007, 0, REG_EDX, 0x00000800,                VENDOR_AMD, "Processor feedback interface"},
	{ 0x80000007, 0, REG_EDX, 0x00001000,                VENDOR_AMD, "Core power reporting"},
	{ 0x80000007, 0, REG_EDX, 0x00002000,                VENDOR_AMD, "Connected standby"},
	{ 0x80000007, 0, REG_EDX, 0x00004000,                VENDOR_AMD, "Running average power limit (RAPL)"},
	{ 0x80000007, 0, REG_EDX, 0x00008000,                VENDOR_AMD, "Fast CPPC"},
/*	{ 0x80000007, 0, REG_EDX, 0x00010000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00020000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00040000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00080000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00100000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00200000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00400000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x00800000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x01000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x02000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x04000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x08000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x10000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x20000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x40000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x80000007, 0, REG_EDX, 0x80000000,                VENDOR_AMD, ""}, */   /* Reserved */

/* Extended Feature Extensions ID (8000_0008h) */
	{ 0x80000008, 0, REG_EBX, 0x00000001,                VENDOR_AMD, "CLZERO instruction"},
	{ 0x80000008, 0, REG_EBX, 0x00000002,                VENDOR_AMD, "Instructions retired count support (IRPerf)"},
	{ 0x80000008, 0, REG_EBX, 0x00000004,                VENDOR_AMD, "XSAVE always saves/restores error pointers"},
	{ 0x80000008, 0, REG_EBX, 0x00000008,                VENDOR_AMD, "INVLPGB and TLBSYNC instruction"},
	{ 0x80000008, 0, REG_EBX, 0x00000010,                VENDOR_AMD, "RDPRU instruction"},
/*	{ 0x80000008, 0, REG_EBX, 0x00000020,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000008, 0, REG_EBX, 0x00000040,                VENDOR_AMD, "Memory bandwidth enforcement (MBE)"},
/*	{ 0x80000008, 0, REG_EBX, 0x00000080,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000008, 0, REG_EBX, 0x00000100,                VENDOR_AMD, "MCOMMIT instruction"},
	{ 0x80000008, 0, REG_EBX, 0x00000200, VENDOR_INTEL | VENDOR_AMD, "WBNOINVD (Write back and do not invalidate cache)"},
	{ 0x80000008, 0, REG_EBX, 0x00000400,                VENDOR_AMD, "LBR extensions"},
/*	{ 0x80000008, 0, REG_EBX, 0x00000800,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000008, 0, REG_EBX, 0x00001000,                VENDOR_AMD, "Indirect Branch Prediction Barrier (IBPB)"},
	{ 0x80000008, 0, REG_EBX, 0x00002000,                VENDOR_AMD, "WBINVD (Write back and invalidate cache)"},
	{ 0x80000008, 0, REG_EBX, 0x00004000,                VENDOR_AMD, "Indirect Branch Restricted Speculation (IBRS)"},
	{ 0x80000008, 0, REG_EBX, 0x00008000,                VENDOR_AMD, "Single Thread Indirect Branch Predictor (STIBP)"},
/*	{ 0x80000008, 0, REG_EBX, 0x00010000,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000008, 0, REG_EBX, 0x00020000,                VENDOR_AMD, "STIBP always on"},
	{ 0x80000008, 0, REG_EBX, 0x00040000,                VENDOR_AMD, "IBRS preferred over software solution"},
	{ 0x80000008, 0, REG_EBX, 0x00080000,                VENDOR_AMD, "IBRS provides Same Mode Protection"},
	{ 0x80000008, 0, REG_EBX, 0x00100000,                VENDOR_AMD, "EFER.LMLSE is unsupported"},
	{ 0x80000008, 0, REG_EBX, 0x00200000,                VENDOR_AMD, "INVLPGB for guest nested translations"},
/*	{ 0x80000008, 0, REG_EBX, 0x00400000,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x80000008, 0, REG_EBX, 0x00800000,                VENDOR_AMD, "Protected Processor Inventory Number (PPIN)"},
	{ 0x80000008, 0, REG_EBX, 0x01000000,                VENDOR_AMD, "Speculative Store Bypass Disable (SSBD)"},
	{ 0x80000008, 0, REG_EBX, 0x02000000,                VENDOR_AMD, "VIRT_SPEC_CTL"},
	{ 0x80000008, 0, REG_EBX, 0x04000000,                VENDOR_AMD, "SSBD no longer needed"},
	{ 0x80000008, 0, REG_EBX, 0x08000000,                VENDOR_AMD, "Collaborative Processor Performance Control (CPPC)"},
	{ 0x80000008, 0, REG_EBX, 0x10000000,                VENDOR_AMD, "Predictive Store Forward Disable (PSFD)"},
	{ 0x80000008, 0, REG_EBX, 0x20000000,                VENDOR_AMD, "Not vulnerable to branch type confusion (BTC_NO)"},
	{ 0x80000008, 0, REG_EBX, 0x40000000,                VENDOR_AMD, "Clears return address predictor with IBPB (IBPB_RET)"},
	{ 0x80000008, 0, REG_EBX, 0x80000000,                VENDOR_AMD, "Branch sampling"},

/* SVM Revision and Feature Identification (8000_000Ah) */
	{ 0x8000000A, 0, REG_EDX, 0x00000001,                VENDOR_AMD, "Nested paging"},
	{ 0x8000000A, 0, REG_EDX, 0x00000002,                VENDOR_AMD, "LBR virtualization"},
	{ 0x8000000A, 0, REG_EDX, 0x00000004,                VENDOR_AMD, "SVM lock"},
	{ 0x8000000A, 0, REG_EDX, 0x00000008,                VENDOR_AMD, "NRIP save"},
	{ 0x8000000A, 0, REG_EDX, 0x00000010,                VENDOR_AMD, "MSR-based TSC rate control"},
	{ 0x8000000A, 0, REG_EDX, 0x00000020,                VENDOR_AMD, "VMCB clean bits"},
	{ 0x8000000A, 0, REG_EDX, 0x00000040,                VENDOR_AMD, "Flush by ASID"},
	{ 0x8000000A, 0, REG_EDX, 0x00000080,                VENDOR_AMD, "Decode assists"},
	{ 0x8000000A, 0, REG_EDX, 0x00000100,                VENDOR_AMD, "Performance Monitor Counter virtualization"},
/*	{ 0x8000000A, 0, REG_EDX, 0x00000200,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x8000000A, 0, REG_EDX, 0x00000400,                VENDOR_AMD, "Pause intercept filter"},
	{ 0x8000000A, 0, REG_EDX, 0x00000800,                VENDOR_AMD, "Encrypted µcode patch"},
	{ 0x8000000A, 0, REG_EDX, 0x00001000,                VENDOR_AMD, "PAUSE filter threshold"},
	{ 0x8000000A, 0, REG_EDX, 0x00002000,                VENDOR_AMD, "AMD virtual interrupt controller (AVIC)"},
/*	{ 0x8000000A, 0, REG_EDX, 0x00004000,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x8000000A, 0, REG_EDX, 0x00008000,                VENDOR_AMD, "Virtualized VMLOAD/VMSAVE"},
	{ 0x8000000A, 0, REG_EDX, 0x00010000,                VENDOR_AMD, "Virtualized GIF"},
	{ 0x8000000A, 0, REG_EDX, 0x00020000,                VENDOR_AMD, "Guest mode execution trap (GMET)"},
	{ 0x8000000A, 0, REG_EDX, 0x00040000,                VENDOR_AMD, "Virtualized X2APIC (X2AVIC)"},
	{ 0x8000000A, 0, REG_EDX, 0x00080000,                VENDOR_AMD, "SVM supervisor shadow stack restrictions"},
	{ 0x8000000A, 0, REG_EDX, 0x00100000,                VENDOR_AMD, "SPEC_CTRL virtualization"},
	{ 0x8000000A, 0, REG_EDX, 0x00200000,                VENDOR_AMD, "Non-writable guest pages for NPT"},
/*	{ 0x8000000A, 0, REG_EDX, 0x00400000,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x8000000A, 0, REG_EDX, 0x00800000,                VENDOR_AMD, "Host MCE override"},
	{ 0x8000000A, 0, REG_EDX, 0x01000000,                VENDOR_AMD, "INVLPGB/TLBSYNC hypervisor enable"},
	{ 0x8000000A, 0, REG_EDX, 0x02000000,                VENDOR_AMD, "Guest NMI virtualization"},
	{ 0x8000000A, 0, REG_EDX, 0x04000000,                VENDOR_AMD, "IBS virtualization"},
	{ 0x8000000A, 0, REG_EDX, 0x08000000,                VENDOR_AMD, "Read-only extended LVT offsets"},
	{ 0x8000000A, 0, REG_EDX, 0x10000000,                VENDOR_AMD, "VMCB address check change"},
	{ 0x8000000A, 0, REG_EDX, 0x20000000,                VENDOR_AMD, "Guest bus lock threshold"},
	{ 0x8000000A, 0, REG_EDX, 0x40000000,                VENDOR_AMD, "HLT idle interception"},
	{ 0x8000000A, 0, REG_EDX, 0x80000000,                VENDOR_AMD, "Enahnced shutdown intercept"},

/* Performance Optimization Identifiers (8000_001Ah) */
	{ 0x8000001A, 0, REG_EAX, 0x00000001,                VENDOR_AMD, "128-bit SSE full-width pipelines (FP128)"},
	{ 0x8000001A, 0, REG_EAX, 0x00000002,                VENDOR_AMD, "Efficient MOVU SSE instructions (MOVU)"},
	{ 0x8000001A, 0, REG_EAX, 0x00000004,                VENDOR_AMD, "256-bit AVX full-width pipelines (FP256)"},
	{ 0x8000001A, 0, REG_EAX, 0x00000008,                VENDOR_AMD, "512-bit AVX full-width pipelines (FP512)"},
/*	{ 0x8000001A, 0, REG_EAX, 0x00000010,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000020,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000040,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000080,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000100,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000200,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000400,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00000800,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00001000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00002000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00004000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00008000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00010000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00020000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00040000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00080000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00100000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00200000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00400000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x00800000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x01000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x02000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x04000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x08000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x10000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x20000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x40000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001A, 0, REG_EAX, 0x80000000,                VENDOR_AMD, ""}, */   /* Reserved */

/* Instruction Based Sampling Identifiers (8000_001Bh) */
	{ 0x8000001B, 0, REG_EAX, 0x00000001,                VENDOR_AMD, "IBS feature flags valid (IBSFFV)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000002,                VENDOR_AMD, "IBS fetch sampling (FetchSam)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000004,                VENDOR_AMD, "IBS execution sampling (OpSam)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000008,                VENDOR_AMD, "Read/write of op counter (RdWrOpCnt)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000010,                VENDOR_AMD, "Op counting mode (OpCnt)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000020,                VENDOR_AMD, "Branch target address reporting (BrnTrgt)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000040,                VENDOR_AMD, "IBS op cur/max count extended by 7 bits (OpCntExt)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000080,                VENDOR_AMD, "IBS RIP invalid indication (RipInvalidChk)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000100,                VENDOR_AMD, "IBS fused branch micro-op indication (OpBrnFuse)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000200,                VENDOR_AMD, "IBS fetch control extended MSR (IbsFetchCtlExtd)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000400,                VENDOR_AMD, "IBS op data 4 MSR (IbsOpData4)"},
	{ 0x8000001B, 0, REG_EAX, 0x00000800,                VENDOR_AMD, "L3 Miss Filtering for IBS supported (IbsL3MissFiltering)"},
	{ 0x8000001B, 0, REG_EAX, 0x00001000,                VENDOR_AMD, "IBS filtering based on load latency (IbsLoadLatencyFiltering)"},
/*	{ 0x8000001B, 0, REG_EAX, 0x00002000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00004000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00008000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00010000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00020000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00040000,                VENDOR_AMD, ""}, */   /* Reserved */
	{ 0x8000001B, 0, REG_EAX, 0x00080000,                VENDOR_AMD, "Simplified DTLB page size and miss reporting (IbsUpdtdDtlbStats)"},
/*	{ 0x8000001B, 0, REG_EAX, 0x00100000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00200000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00400000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x00800000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x01000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x02000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x04000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x08000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x10000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x20000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x40000000,                VENDOR_AMD, ""}, */   /* Reserved */
/*	{ 0x8000001B, 0, REG_EAX, 0x80000000,                VENDOR_AMD, ""}, */   /* Reserved */

/* Centaur features (c000_0001h) */
	{ 0xc0000001, 0, REG_EDX, 0x00000001, VENDOR_CENTAUR           , "Alternate Instruction Set available"},
	{ 0xc0000001, 0, REG_EDX, 0x00000002, VENDOR_CENTAUR           , "Alternate Instruction Set enabled"},
	{ 0xc0000001, 0, REG_EDX, 0x00000004, VENDOR_CENTAUR           , "Random Number Generator available"},
	{ 0xc0000001, 0, REG_EDX, 0x00000008, VENDOR_CENTAUR           , "Random Number Generator enabled"},
	{ 0xc0000001, 0, REG_EDX, 0x00000010, VENDOR_CENTAUR           , "LongHaul MSR 0000_110Ah"},
	{ 0xc0000001, 0, REG_EDX, 0x00000020, VENDOR_CENTAUR           , "FEMMS"},
	{ 0xc0000001, 0, REG_EDX, 0x00000040, VENDOR_CENTAUR           , "Advanced Cryptography Engine (ACE) available"},
	{ 0xc0000001, 0, REG_EDX, 0x00000080, VENDOR_CENTAUR           , "Advanced Cryptography Engine (ACE) enabled"},
	{ 0xc0000001, 0, REG_EDX, 0x00000100, VENDOR_CENTAUR           , "Montgomery Multiplier and Hash Engine (ACE2) available"},
	{ 0xc0000001, 0, REG_EDX, 0x00000200, VENDOR_CENTAUR           , "Montgomery Multiplier and Hash Engine (ACE2) enabled"},
	{ 0xc0000001, 0, REG_EDX, 0x00000400, VENDOR_CENTAUR           , "Padlock hash engine (PHE) available"},
	{ 0xc0000001, 0, REG_EDX, 0x00000800, VENDOR_CENTAUR           , "Padlock hash engine (PHE) enabled"},
	{ 0xc0000001, 0, REG_EDX, 0x00001000, VENDOR_CENTAUR           , "Padlock montgomery multiplier (PMM) available"},
	{ 0xc0000001, 0, REG_EDX, 0x00002000, VENDOR_CENTAUR           , "Padlock montgomery multiplier (PMM) enabled"},

	{ 0, 0, REG_NULL, 0, 0, NULL}
};

static const char *vendors(char *buffer, uint32_t mask)
{
	char multi = 0;
	buffer[0] = 0;
	if (mask & VENDOR_INTEL) {
		if (multi)
			strcat(buffer, ", ");
		strcat(buffer, "Intel");
		multi = 1;
	}
	if (mask & VENDOR_AMD) {
		if (multi)
			strcat(buffer, ", ");
		strcat(buffer, "AMD");
		multi = 1;
	}
	if (mask & VENDOR_TRANSMETA) {
		if (multi)
			strcat(buffer, ", ");
		strcat(buffer, "Transmeta");
		multi = 1;
	}
	if (mask & VENDOR_CENTAUR) {
		if (multi)
			strcat(buffer, ", ");
		strcat(buffer, "Centaur");
		multi = 1;
	}
	return buffer;
}

int print_features(const struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	int leaf_checked = 0;
	int flags_found = 0;
	int ignore_vendor = state->ignore_vendor;
	const struct cpu_feature_t *p = features;
	struct cpu_regs_t accounting;
	cpu_register_t last_reg = REG_NULL;
	memcpy(&accounting, regs, sizeof(struct cpu_regs_t));
	while (p && p->m_reg != REG_NULL) {
		const uint32_t *reg;
		uint32_t *acct_reg;

		if (state->last_leaf.eax != p->m_level) {
			p++;
			continue;
		}
		if (state->last_leaf.ecx != p->m_index) {
			p++;
			continue;
		}

		reg = &regs->regs[p->m_reg];
		if (!*reg) {
			p++;
			continue;
		}
		acct_reg = &accounting.regs[p->m_reg];

		if (last_reg != p->m_reg) {
			last_reg = p->m_reg;

			switch(state->last_leaf.eax) {
			case 0x00000001:
				printf("Base features, %s:\n",
				       reg_name(last_reg));

				/* EAX and EBX don't contain feature bits. We should zero these
				 * out so they don't appear to be unaccounted for.
				 */
				accounting.eax = accounting.ebx = 0;
				break;
			case 0x00000006:
				accounting.ebx = accounting.edx = 0;

				/* Bits 15-08 are the number of Intel thread director classes
				 * supported by the processor
				 */
				accounting.ecx &= ~0xff00;
				break;
			case 0x00000007:
				printf("Structured extended feature flags (ecx=%d), %s:\n",
				       state->last_leaf.ecx, reg_name(last_reg));

				/* Clear EAX, which indicates the highest-supported leaf 0x7
				 * subleaf
				 */
				if (state->last_leaf.ecx == 0)
					accounting.eax = 0;

				/* Clear "value of MAWAU used by the BNDLDX and BNDSTX
				 * instructions in 64-bit mode"
				 */
				if (state->last_leaf.ecx == 0)
					accounting.ecx &= ~0x3e0000;
				break;
			case 0x00000014:
				accounting.eax = 0;
				break;
			case 0x40000001:
				printf("KVM features, %s:\n",
				       reg_name(last_reg));
				break;
			case 0x40000003:
				printf("Hyper-V %sfeatures, %s:\n",
				       last_reg == REG_EBX ? "partition " : "",
					   reg_name(last_reg));
				break;
			case 0x40000004:
				if (state->vendor & VENDOR_HV_XEN) {
					printf("Xen HVM-specific features, %s:\n", reg_name(last_reg));

					/* We only look at EAX for this leaf. */
					accounting.ebx = 0;
					accounting.ecx = 0;
					accounting.edx = 0;
				} else if (state->vendor & VENDOR_HV_HYPERV) {
					printf("Hyper-V implementation recommendations, %s:\n",
						   reg_name(last_reg));

					/* EBX doesn't contain feature bits. We should zero these
					 * out so they don't appear to be unaccounted for.
					 */
					accounting.ebx = 0;

					/* ECX[0:6] are not feature bits. */
					accounting.ecx &= ~0x3f;
				}

				break;
			case 0x40000006:
				printf("Hyper-V hardware features detected and in use, %s:\n",
					   reg_name(last_reg));
				break;
			case 0x40000007:
				/* Clear ReservedIdentityBit */
				accounting.eax &= ~0x80000000;
				break;
			case 0x40000008:
				printf("Hyper-V shared virtual memory features, %s:\n",
					   reg_name(last_reg));

				/* The top 21 bits are not flags */
				accounting.eax &= 0x3FF;

				/* EBX, ECX, and EDX do not contain feature flags. */
				accounting.ebx = accounting.ecx = accounting.edx = 0;
				break;
			case 0x80000001:
				printf("Extended features, %s:\n",
				       reg_name(last_reg));

#if !defined(SHOW_REDUNDANT)
				accounting.edx &= ~0x0183FFFF;
#endif

				/* EAX and EBX don't contain feature bits. We should zero these
				 * out so they don't appear to be unaccounted for.
				 */
				accounting.eax = accounting.ebx = 0;
				break;
			case 0x80000007:
				if (p->m_reg == REG_EBX)
					printf("RAS capabilities, %s:\n",
						   reg_name(last_reg));
				else if (p->m_reg == REG_EDX)
					printf("Advanced Power Management features, %s:\n",
						   reg_name(last_reg));
				break;
			case 0x80000008:
				if (p->m_reg == REG_EBX) {
					printf("Extended Feature Extensions:\n");
					accounting.eax = accounting.ecx = accounting.edx = 0;
				}
				break;
			case 0x8000000A:
				if (p->m_reg == REG_EDX) {
					printf("SVM Feature Flags:\n");
					accounting.eax = accounting.ebx = accounting.ecx = 0;
				}
				break;
			case 0xC0000001:
				if (p->m_reg == REG_EDX) {
					printf("Centaur Feature Flags:\n");
					accounting.eax = 0;
				}
			}
		}

		if (state->last_leaf.eax < 0x40000000 || state->last_leaf.eax > 0x4fff0000) {
			int cpu_vendor = state->vendor & VENDOR_CPU_MASK;
			/* Non-hypervisor leaves */
			if (cpu_vendor != VENDOR_AMD && cpu_vendor != VENDOR_INTEL) {
				/* Unusual CPU vendor, just ignore vendor matching and print
				 * any matching feature flags
				 */
				ignore_vendor = 1;
			}
		}

		leaf_checked = 1;

		if (ignore_vendor) {
			if ((*acct_reg & p->m_bitmask) != 0)
			{
				char feat[96], vendorlist[32];
				if (p->m_name) {
					snprintf(feat, sizeof(feat), "%s (%s)", p->m_name, vendors(vendorlist, p->m_vendor));
					printf("  %s\n", feat);
				}
				*acct_reg &= (~p->m_bitmask);
				flags_found++;
			}
		} else {
			if (((int)p->m_vendor == VENDOR_ANY || (state->vendor & p->m_vendor) != 0)
				&& (*acct_reg & p->m_bitmask) != 0)
			{
				printf("  %s\n", p->m_name);
				*acct_reg &= (~p->m_bitmask);
				flags_found++;
			}
		}
		p++;
	}

	if (leaf_checked && (accounting.eax || accounting.ebx || accounting.ecx || accounting.edx))
		printf("Unaccounted for in 0x%08x:0x%08x:\n  eax:0x%08x ebx:0x%08x ecx:0x%08x edx:0x%08x\n",
			state->last_leaf.eax, state->last_leaf.ecx,
		    accounting.eax, accounting.ebx, accounting.ecx, accounting.edx);

	return flags_found;
}

/* vim: set ts=4 sts=4 sw=4 noet: */
