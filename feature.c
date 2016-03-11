/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2015, Steven Noonan <steven@uplinklabs.net>
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
	{ 0x00000001, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "x87 FPU on chip"},
	{ 0x00000001, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "virtual-8086 mode enhancement"},
	{ 0x00000001, 0, REG_EDX, 0x00000004, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "debugging extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000008, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "page size extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "time stamp counter"},
	{ 0x00000001, 0, REG_EDX, 0x00000020, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "RDMSR and WRMSR support"},
	{ 0x00000001, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , "physical address extensions"},
	{ 0x00000001, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD                   , "machine check exception"},
	{ 0x00000001, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "CMPXCHG8B instruction"},
	{ 0x00000001, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , "APIC on chip"},
/*	{ 0x00000001, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "SYSENTER and SYSEXIT instructions"},
	{ 0x00000001, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , "memory type range registers"},
	{ 0x00000001, 0, REG_EDX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , "PTE global bit"},
	{ 0x00000001, 0, REG_EDX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , "machine check architecture"},
	{ 0x00000001, 0, REG_EDX, 0x00008000, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "conditional move instruction"},
	{ 0x00000001, 0, REG_EDX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , "page attribute table"},
	{ 0x00000001, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , "36-bit page size extension"},
	{ 0x00000001, 0, REG_EDX, 0x00040000, VENDOR_INTEL              | VENDOR_TRANSMETA, "processor serial number"},
	{ 0x00000001, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , "CLFLUSH instruction"},
/*	{ 0x00000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL                                , "debug store"},
	{ 0x00000001, 0, REG_EDX, 0x00400000, VENDOR_INTEL                                , "ACPI"},
	{ 0x00000001, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "MMX instruction set"},
	{ 0x00000001, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , "FXSAVE/FXRSTOR instructions"},
	{ 0x00000001, 0, REG_EDX, 0x02000000, VENDOR_INTEL | VENDOR_AMD                   , "SSE instructions"},
	{ 0x00000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "SSE2 instructions"},
	{ 0x00000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL                                , "self snoop"},
	{ 0x00000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , "max APIC IDs reserved field is valid"},
	{ 0x00000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL                                , "thermal monitor"},
/*	{ 0x00000001, 0, REG_EDX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x80000000, VENDOR_INTEL                                , "pending break enable"},

	{ 0x00000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , "SSE3 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000002, VENDOR_INTEL | VENDOR_AMD                   , "PCLMULQDQ instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00000004, VENDOR_INTEL                                , "64-bit DS area"},
	{ 0x00000001, 0, REG_ECX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , "MONITOR/MWAIT instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000010, VENDOR_INTEL                                , "CPL qualified debug store"},
	{ 0x00000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL                                , "virtual machine extensions"},
	{ 0x00000001, 0, REG_ECX, 0x00000040, VENDOR_INTEL                                , "safer mode extensions"},
	{ 0x00000001, 0, REG_ECX, 0x00000080, VENDOR_INTEL                                , "Enhanced Intel SpeedStep"},
	{ 0x00000001, 0, REG_ECX, 0x00000100, VENDOR_INTEL                                , "thermal monitor 2"},
	{ 0x00000001, 0, REG_ECX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , "SSSE3 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00000400, VENDOR_INTEL                                , "L1 context ID"},
	{ 0x00000001, 0, REG_ECX, 0x00000800, VENDOR_INTEL                                , "silicon debug"}, /* supports IA32_DEBUG_INTERFACE MSR for silicon debug */
	{ 0x00000001, 0, REG_ECX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , "fused multiply-add AVX instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , "CMPXCHG16B instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL                                , "xTPR update control"},
	{ 0x00000001, 0, REG_ECX, 0x00008000, VENDOR_INTEL                                , "perfmon and debug capability"},
/*	{ 0x00000001, 0, REG_ECX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , "process-context identifiers"},
	{ 0x00000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL                                , "direct cache access"},
	{ 0x00000001, 0, REG_ECX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , "SSE4.1 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , "SSE4.2 instructions"},
	{ 0x00000001, 0, REG_ECX, 0x00200000, VENDOR_INTEL                                , "x2APIC"},
	{ 0x00000001, 0, REG_ECX, 0x00400000, VENDOR_INTEL                                , "MOVBE instruction"},
	{ 0x00000001, 0, REG_ECX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , "POPCNT instruction"},
	{ 0x00000001, 0, REG_ECX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , "TSC deadline"},
	{ 0x00000001, 0, REG_ECX, 0x02000000, VENDOR_INTEL                                , "AES instructions"},
	{ 0x00000001, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "XSAVE/XRSTOR instructions"},
	{ 0x00000001, 0, REG_ECX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , "OS-enabled XSAVE/XRSTOR"},
	{ 0x00000001, 0, REG_ECX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , "AVX instructions"},
	{ 0x00000001, 0, REG_ECX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , "16-bit FP conversion instructions"},
	{ 0x00000001, 0, REG_ECX, 0x40000000, VENDOR_INTEL                                , "RDRAND instruction"},
	{ 0x00000001, 0, REG_ECX, 0x80000000, VENDOR_ANY                                  , "RAZ (hypervisor)"},

/*  Structured Extended Feature Flags (0000_0007h) */
	{ 0x00000007, 0, REG_EBX, 0x00000001, VENDOR_INTEL                                , "FSGSBASE instructions"},
	{ 0x00000007, 0, REG_EBX, 0x00000002, VENDOR_INTEL                                , "IA32_TSC_ADJUST MSR supported"},
	{ 0x00000007, 0, REG_EBX, 0x00000004, VENDOR_INTEL                                , "Software Guard Extensions (SGX)"},
	{ 0x00000007, 0, REG_EBX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , "Bit Manipulation Instructions (BMI1)"},
	{ 0x00000007, 0, REG_EBX, 0x00000010, VENDOR_INTEL                                , "Hardware Lock Elision (HLE)"},
	{ 0x00000007, 0, REG_EBX, 0x00000020, VENDOR_INTEL                                , "Advanced Vector Extensions 2.0 (AVX2)"},
/*	{ 0x00000007, 0, REG_EBX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x00000080, VENDOR_INTEL                                , "Supervisor Mode Execution Protection (SMEP)"},
	{ 0x00000007, 0, REG_EBX, 0x00000100, VENDOR_INTEL                                , "Bit Manipulation Instructions 2 (BMI2)"},
	{ 0x00000007, 0, REG_EBX, 0x00000200, VENDOR_INTEL                                , "Enhanced REP MOVSB/STOSB"},
	{ 0x00000007, 0, REG_EBX, 0x00000400, VENDOR_INTEL                                , "INVPCID instruction"},
	{ 0x00000007, 0, REG_EBX, 0x00000800, VENDOR_INTEL                                , "Restricted Transactional Memory (RTM)"},
	{ 0x00000007, 0, REG_EBX, 0x00001000, VENDOR_INTEL                                , "Platform QoS Monitoring (PQM)"},
	{ 0x00000007, 0, REG_EBX, 0x00002000, VENDOR_INTEL                                , "x87 FPU CS and DS deprecated"},
	{ 0x00000007, 0, REG_EBX, 0x00004000, VENDOR_INTEL                                , "Memory Protection Extensions (MPX)"},
	{ 0x00000007, 0, REG_EBX, 0x00008000, VENDOR_INTEL                                , "Platform QoS Enforcement (PQE)"},
	{ 0x00000007, 0, REG_EBX, 0x00010000, VENDOR_INTEL                                , "AVX512 foundation (AVX512F)"},
	{ 0x00000007, 0, REG_EBX, 0x00020000, VENDOR_INTEL                                , "AVX512 double/quadword instructions (AVX512DQ)"},
	{ 0x00000007, 0, REG_EBX, 0x00040000, VENDOR_INTEL                                , "RDSEED instruction"},
	{ 0x00000007, 0, REG_EBX, 0x00080000, VENDOR_INTEL                                , "Multi-Precision Add-Carry Instruction Extensions (ADX)"},
	{ 0x00000007, 0, REG_EBX, 0x00100000, VENDOR_INTEL                                , "Supervisor Mode Access Prevention (SMAP)"},
	{ 0x00000007, 0, REG_EBX, 0x00200000, VENDOR_INTEL                                , "AVX512 integer FMA instructions (AVX512IFMA)"},
	{ 0x00000007, 0, REG_EBX, 0x00400000, VENDOR_INTEL                                , "Persistent commit instruction (PCOMMIT)"},
	{ 0x00000007, 0, REG_EBX, 0x00800000, VENDOR_INTEL                                , "CLFLUSHOPT instruction"},
	{ 0x00000007, 0, REG_EBX, 0x01000000, VENDOR_INTEL                                , "cache line write-back instruction (CLWB)"},
	{ 0x00000007, 0, REG_EBX, 0x02000000, VENDOR_INTEL                                , "Intel Processor Trace"},
	{ 0x00000007, 0, REG_EBX, 0x04000000, VENDOR_INTEL                                , "AVX512 prefetch instructions (AVX512PF)"},
	{ 0x00000007, 0, REG_EBX, 0x08000000, VENDOR_INTEL                                , "AVX512 exponent/reciprocal instructions (AVX512ER)"},
	{ 0x00000007, 0, REG_EBX, 0x10000000, VENDOR_INTEL                                , "AVX512 conflict detection instructions (AVX512CD)"},
	{ 0x00000007, 0, REG_EBX, 0x20000000, VENDOR_INTEL                                , "SHA-1/SHA-256 instructions"},
	{ 0x00000007, 0, REG_EBX, 0x40000000, VENDOR_INTEL                                , "AVX512 byte/word instructions (AVX512BW)"},
	{ 0x00000007, 0, REG_EBX, 0x80000000, VENDOR_INTEL                                , "AVX512 vector length extensions (AVX512VL)"},

	{ 0x00000007, 0, REG_ECX, 0x00000001, VENDOR_INTEL                                , "PREFETCHWT1 instruction"},
	{ 0x00000007, 0, REG_ECX, 0x00000002, VENDOR_INTEL                                , "AVX512 vector byte manipulation instructions (AVX512VBMI)"},
/*	{ 0x00000007, 0, REG_ECX, 0x00000004, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000010, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000020, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000080, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000100, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000400, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000800, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00008000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00040000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00200000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00400000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x02000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x80000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */

/*	{ 0x00000007, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000004, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000020, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00008000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00040000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00200000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00400000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x02000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EDX, 0x80000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */


/*  Hypervisor (4000_0001h) */
	{ 0x40000001, 0, REG_EAX, 0x00000001, VENDOR_HV_KVM                               , "Clocksource"},
	{ 0x40000001, 0, REG_EAX, 0x00000002, VENDOR_HV_KVM                               , "NOP IO Delay"},
	{ 0x40000001, 0, REG_EAX, 0x00000004, VENDOR_HV_KVM                               , "MMU Op"},
	{ 0x40000001, 0, REG_EAX, 0x00000008, VENDOR_HV_KVM                               , "Clocksource 2"},
	{ 0x40000001, 0, REG_EAX, 0x00000010, VENDOR_HV_KVM                               , "Async PF"},
	{ 0x40000001, 0, REG_EAX, 0x00000020, VENDOR_HV_KVM                               , "Steal Time"},
	{ 0x40000001, 0, REG_EAX, 0x00000040, VENDOR_HV_KVM                               , "PV EOI"},
	{ 0x40000001, 0, REG_EAX, 0x00000080, VENDOR_HV_KVM                               , "PV UNHALT"},
/*	{ 0x40000001, 0, REG_EAX, 0x00000100,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00000200,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00000400,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00000800,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00001000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00002000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00004000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00008000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00010000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00020000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00040000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00080000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00100000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00200000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00400000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x00800000,                                             , ""}, */   /* Reserved */
	{ 0x40000001, 0, REG_EAX, 0x01000000, VENDOR_HV_KVM                               , "Clocksource Stable"},
/*	{ 0x40000001, 0, REG_EAX, 0x02000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x04000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x08000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x10000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x20000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x40000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000001, 0, REG_EAX, 0x80000000,                                             , ""}, */   /* Reserved */

/*  Hypervisor (4000_0003h) */
	{ 0x40000003, 0, REG_EAX, 0x00000001, VENDOR_HV_HYPERV                            , "VP_RUNTIME"},
	{ 0x40000003, 0, REG_EAX, 0x00000002, VENDOR_HV_HYPERV                            , "TIME_REF_COUNT"},
	{ 0x40000003, 0, REG_EAX, 0x00000004, VENDOR_HV_HYPERV                            , "Basic SynIC MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000008, VENDOR_HV_HYPERV                            , "Synthetic Timer"},
	{ 0x40000003, 0, REG_EAX, 0x00000010, VENDOR_HV_HYPERV                            , "APIC access"},
	{ 0x40000003, 0, REG_EAX, 0x00000020, VENDOR_HV_HYPERV                            , "Hypercall MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000040, VENDOR_HV_HYPERV                            , "VP Index MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000080, VENDOR_HV_HYPERV                            , "System Reset MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000100, VENDOR_HV_HYPERV                            , "Access stats MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00000200, VENDOR_HV_HYPERV                            , "Reference TSC"},
	{ 0x40000003, 0, REG_EAX, 0x00000400, VENDOR_HV_HYPERV                            , "Guest Idle MSR"},
	{ 0x40000003, 0, REG_EAX, 0x00000800, VENDOR_HV_HYPERV                            , "Timer Frequency MSRs"},
	{ 0x40000003, 0, REG_EAX, 0x00001000, VENDOR_HV_HYPERV                            , "Debug MSRs"},
/*	{ 0x40000003, 0, REG_EAX, 0x00002000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00004000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00008000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00010000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00020000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00040000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00080000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00100000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00200000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00400000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x00800000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x01000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x02000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x04000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x08000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x10000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x20000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x40000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EAX, 0x80000000,                                             , ""}, */   /* Reserved */

	{ 0x40000003, 0, REG_EDX, 0x00000001, VENDOR_HV_HYPERV                            , "MWAIT instruction support"},
	{ 0x40000003, 0, REG_EDX, 0x00000002, VENDOR_HV_HYPERV                            , "Guest debugging support"},
	{ 0x40000003, 0, REG_EDX, 0x00000004, VENDOR_HV_HYPERV                            , "Performance Monitor support"},
	{ 0x40000003, 0, REG_EDX, 0x00000008, VENDOR_HV_HYPERV                            , "Physical CPU dynamic partitioning event support"},
	{ 0x40000003, 0, REG_EDX, 0x00000010, VENDOR_HV_HYPERV                            , "Hypercall via XMM registers"},
	{ 0x40000003, 0, REG_EDX, 0x00000020, VENDOR_HV_HYPERV                            , "Virtual guest idle state support"},
	{ 0x40000003, 0, REG_EDX, 0x00000040, VENDOR_HV_HYPERV                            , "Hypervisor sleep state support"},
	{ 0x40000003, 0, REG_EDX, 0x00000080, VENDOR_HV_HYPERV                            , "NUMA distance query support"},
	{ 0x40000003, 0, REG_EDX, 0x00000100, VENDOR_HV_HYPERV                            , "Timer frequency details available"},
	{ 0x40000003, 0, REG_EDX, 0x00000200, VENDOR_HV_HYPERV                            , "Synthetic machine check injection support"},
	{ 0x40000003, 0, REG_EDX, 0x00000400, VENDOR_HV_HYPERV                            , "Guest crash MSR support"},
	{ 0x40000003, 0, REG_EDX, 0x00000800, VENDOR_HV_HYPERV                            , "Debug MSR support"},
	{ 0x40000003, 0, REG_EDX, 0x00001000, VENDOR_HV_HYPERV                            , "NPIEP support"},
	{ 0x40000003, 0, REG_EDX, 0x00002000, VENDOR_HV_HYPERV                            , "Hypervisor disable support"},
/*	{ 0x40000003, 0, REG_EDX, 0x00004000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00008000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00010000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00020000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00040000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00080000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00100000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00200000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00400000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x00800000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x01000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x02000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x04000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x08000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x10000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x20000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x40000000,                                             , ""}, */   /* Reserved */
/*	{ 0x40000003, 0, REG_EDX, 0x80000000,                                             , ""}, */   /* Reserved */

/*  Extended (8000_0001h) */
/*	{ 0x80000001, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000004, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000020, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD                   , "SYSCALL"},
/*	{ 0x80000001, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00008000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00040000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL                                , "XD bit"},
	{ 0x80000001, 0, REG_EDX, 0x00100000,                VENDOR_AMD                   , "NX bit"},
/*	{ 0x80000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00400000,                VENDOR_AMD                   , "MMX extended"},
/*	{ 0x80000001, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x02000000,                VENDOR_AMD                   , "fast FXSAVE/FXRSTOR"},
	{ 0x80000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "1GB page support"},
	{ 0x80000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , "RDTSCP instruction"},
/*	{ 0x80000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , "long mode (EM64T)"},
	{ 0x80000001, 0, REG_EDX, 0x40000000,                VENDOR_AMD                   , "3DNow! extended"},
	{ 0x80000001, 0, REG_EDX, 0x80000000,                VENDOR_AMD                   , "3DNow! instructions"},

	{ 0x80000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , "LAHF/SAHF supported in 64-bit mode"},
	{ 0x80000001, 0, REG_ECX, 0x00000002,                VENDOR_AMD                   , "core multi-processing legacy mode"},
	{ 0x80000001, 0, REG_ECX, 0x00000004,                VENDOR_AMD                   , "secure virtual machine (SVM)"},
	{ 0x80000001, 0, REG_ECX, 0x00000008,                VENDOR_AMD                   , "extended APIC space"},
	{ 0x80000001, 0, REG_ECX, 0x00000010,                VENDOR_AMD                   , "AltMovCr8"},
	{ 0x80000001, 0, REG_ECX, 0x00000020,                VENDOR_AMD                   , "advanced bit manipulation"},
	{ 0x80000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL                                , "LZCNT instruction"},
	{ 0x80000001, 0, REG_ECX, 0x00000040,                VENDOR_AMD                   , "SSE4A instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00000080,                VENDOR_AMD                   , "mis-aligned SSE support"},
	{ 0x80000001, 0, REG_ECX, 0x00000100, VENDOR_INTEL | VENDOR_AMD                   , "3DNow! prefetch instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00000200,                VENDOR_AMD                   , "os-visible workaround"},
	{ 0x80000001, 0, REG_ECX, 0x00000400,                VENDOR_AMD                   , "instruction-based sampling"},
	{ 0x80000001, 0, REG_ECX, 0x00000800,                VENDOR_AMD                   , "extended operations"},
	{ 0x80000001, 0, REG_ECX, 0x00001000,                VENDOR_AMD                   , "SKINIT/STGI instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00002000,                VENDOR_AMD                   , "watchdog timer"},
/*	{ 0x80000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00008000,                VENDOR_AMD                   , "lightweight profiling"},
	{ 0x80000001, 0, REG_ECX, 0x00010000,                VENDOR_AMD                   , "4-operand FMA instructions"},
/*	{ 0x80000001, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00080000,                VENDOR_AMD                   , "node ID support"},
/*	{ 0x80000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00200000,                VENDOR_AMD                   , "trailing bit manipulation instructions"},
	{ 0x80000001, 0, REG_ECX, 0x00400000,                VENDOR_AMD                   , "topology extensions"},
/*	{ 0x80000001, 0, REG_ECX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x02000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x80000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */

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
	return buffer;
}

void print_features(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	const struct cpu_feature_t *p = features;
	cpu_register_t last_reg = REG_NULL;
	while (p && p->m_reg != REG_NULL) {
		uint32_t *reg;

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

		if (last_reg != p->m_reg) {
			last_reg = p->m_reg;

			switch(state->last_leaf.eax) {
			case 0x00000001:
				printf("Base features, %s:\n",
				       reg_name(last_reg));

				/* EAX and EBX don't contain feature bits. We should zero these
				 * out so they don't appear to be unaccounted for.
				 */
				regs->eax = regs->ebx = 0;
				break;
			case 0x00000007:
				printf("Structured extended feature flags (ecx=%d), %s:\n",
				       state->last_leaf.ecx, reg_name(last_reg));
				break;
			case 0x40000001:
				printf("KVM features, %s:\n",
				       reg_name(last_reg));
				break;
			case 0x40000003:
				printf("Hyper-V features, %s:\n",
				       reg_name(last_reg));
				break;
			case 0x80000001:
				printf("Extended features, %s:\n",
				       reg_name(last_reg));
				break;
			}
		}

		if (ignore_vendor) {
			if ((*reg & p->m_bitmask) != 0)
			{
				char feat[32], buffer[32];
				sprintf(feat, "%s (%s)", p->m_name, vendors(buffer, p->m_vendor));
				printf("  %s\n", feat);
				*reg &= (~p->m_bitmask);
			}
		} else {
			if (((int)p->m_vendor == VENDOR_ANY || (state->vendor & p->m_vendor) != 0)
				&& (*reg & p->m_bitmask) != 0)
			{
				printf("  %s\n", p->m_name);
				*reg &= (~p->m_bitmask);
			}
		}
		p++;
	}

	if (regs->eax || regs->ebx || regs->ecx || regs->edx)
		printf("Unaccounted for in 0x%08x:0x%08x:\n  eax: 0x%08x ebx:0x%08x ecx:0x%08x edx:0x%08x\n",
			state->last_leaf.eax, state->last_leaf.ecx,
		    regs->eax, regs->ebx, regs->ecx, regs->edx);
}
