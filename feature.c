/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2013, Steven Noonan <steven@uplinklabs.net>
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
	REG_NULL = 0,
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX
} cpu_register_t;

struct cpu_feature_t
{
	uint32_t m_level;
	uint32_t m_index;
	uint8_t  m_reg;
	uint32_t m_bitmask;
	uint32_t m_vendor;
	const char *m_name;
};

static const struct cpu_feature_t features [] = {
/*  Standard (0000_0001h) */
	{ 0x00000001, 0, REG_EDX, 0x00000001, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "FPU"},
	{ 0x00000001, 0, REG_EDX, 0x00000002, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "VME"},
	{ 0x00000001, 0, REG_EDX, 0x00000004, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "DE"},
	{ 0x00000001, 0, REG_EDX, 0x00000008, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "PSE"},
	{ 0x00000001, 0, REG_EDX, 0x00000010, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "TSC"},
	{ 0x00000001, 0, REG_EDX, 0x00000020, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "MSR"},
	{ 0x00000001, 0, REG_EDX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , "PAE"},
	{ 0x00000001, 0, REG_EDX, 0x00000080, VENDOR_INTEL | VENDOR_AMD                   , "MCE"},
	{ 0x00000001, 0, REG_EDX, 0x00000100, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "CX8"},
	{ 0x00000001, 0, REG_EDX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , "APIC"},
/*	{ 0x00000001, 0, REG_EDX, 0x00000400, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00000800, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "SEP"},
	{ 0x00000001, 0, REG_EDX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , "MTRR"},
	{ 0x00000001, 0, REG_EDX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , "PGE"},
	{ 0x00000001, 0, REG_EDX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , "MCA"},
	{ 0x00000001, 0, REG_EDX, 0x00008000, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "CMOV"},
	{ 0x00000001, 0, REG_EDX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , "PAT"},
	{ 0x00000001, 0, REG_EDX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , "PSE-36"},
	{ 0x00000001, 0, REG_EDX, 0x00040000, VENDOR_INTEL              | VENDOR_TRANSMETA, "PSN"},
	{ 0x00000001, 0, REG_EDX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , "CLFSH"},
/*	{ 0x00000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL                                , "DS"},
	{ 0x00000001, 0, REG_EDX, 0x00400000, VENDOR_INTEL                                , "ACPI"},
	{ 0x00000001, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD | VENDOR_TRANSMETA, "MMX"},
	{ 0x00000001, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , "FXSR"},
	{ 0x00000001, 0, REG_EDX, 0x02000000, VENDOR_INTEL | VENDOR_AMD                   , "SSE"},
	{ 0x00000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "SSE2"},
	{ 0x00000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL                                , "SS"},
	{ 0x00000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , "HTT"},
	{ 0x00000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL                                , "TM"},
/*	{ 0x00000001, 0, REG_EDX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_EDX, 0x80000000, VENDOR_INTEL                                , "PBE"},

	{ 0x00000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , "SSE3"},
	{ 0x00000001, 0, REG_ECX, 0x00000002, VENDOR_INTEL | VENDOR_AMD                   , "PCLMULQDQ"},
	{ 0x00000001, 0, REG_ECX, 0x00000004, VENDOR_INTEL                                , "DTES64"},
	{ 0x00000001, 0, REG_ECX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , "MONITOR"},
	{ 0x00000001, 0, REG_ECX, 0x00000010, VENDOR_INTEL                                , "DS-CPL"},
	{ 0x00000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL                                , "VMX"},
	{ 0x00000001, 0, REG_ECX, 0x00000040, VENDOR_INTEL                                , "SMX"},
	{ 0x00000001, 0, REG_ECX, 0x00000080, VENDOR_INTEL                                , "EST"},
	{ 0x00000001, 0, REG_ECX, 0x00000100, VENDOR_INTEL                                , "TM2"},
	{ 0x00000001, 0, REG_ECX, 0x00000200, VENDOR_INTEL | VENDOR_AMD                   , "SSSE3"},
	{ 0x00000001, 0, REG_ECX, 0x00000400, VENDOR_INTEL                                , "CNXT-ID"},
	{ 0x00000001, 0, REG_ECX, 0x00000800, VENDOR_INTEL                                , "SDBG"}, /* supports IA32_DEBUG_INTERFACE MSR for silicon debug */
	{ 0x00000001, 0, REG_ECX, 0x00001000, VENDOR_INTEL | VENDOR_AMD                   , "FMA"},
	{ 0x00000001, 0, REG_ECX, 0x00002000, VENDOR_INTEL | VENDOR_AMD                   , "CX16"},
	{ 0x00000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL                                , "xTPR"},
	{ 0x00000001, 0, REG_ECX, 0x00008000, VENDOR_INTEL                                , "PDCM"},
/*	{ 0x00000001, 0, REG_ECX, 0x00010000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000001, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , "PCID"},
	{ 0x00000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL                                , "DCA"},
	{ 0x00000001, 0, REG_ECX, 0x00080000, VENDOR_INTEL | VENDOR_AMD                   , "SSE4.1"},
	{ 0x00000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , "SSE4.2"},
	{ 0x00000001, 0, REG_ECX, 0x00200000, VENDOR_INTEL                                , "x2APIC"},
	{ 0x00000001, 0, REG_ECX, 0x00400000, VENDOR_INTEL                                , "MOVBE"},
	{ 0x00000001, 0, REG_ECX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , "POPCNT"},
	{ 0x00000001, 0, REG_ECX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , "TSC-Deadline"},
	{ 0x00000001, 0, REG_ECX, 0x02000000, VENDOR_INTEL                                , "AES"},
	{ 0x00000001, 0, REG_ECX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "XSAVE"},
	{ 0x00000001, 0, REG_ECX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , "OSXSAVE"},
	{ 0x00000001, 0, REG_ECX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , "AVX"},
	{ 0x00000001, 0, REG_ECX, 0x20000000, VENDOR_INTEL | VENDOR_AMD                   , "F16C"},
	{ 0x00000001, 0, REG_ECX, 0x40000000, VENDOR_INTEL                                , "RDRAND"},
	{ 0x00000001, 0, REG_ECX, 0x80000000, VENDOR_ANY                                  , "RAZ"},

/*  Structured Extended Feature Flags (0000_0007h) */
	{ 0x00000007, 0, REG_EBX, 0x00000001, VENDOR_INTEL                                , "FSGSBASE"}, /* {RD,WR}{FS,GS}BASE */
	{ 0x00000007, 0, REG_EBX, 0x00000002, VENDOR_INTEL                                , "TSC_ADJUST"}, /* IA32_TSC_ADJUST MSR */
/*	{ 0x00000007, 0, REG_EBX, 0x00000004, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x00000008, VENDOR_INTEL | VENDOR_AMD                   , "BMI1"},
	{ 0x00000007, 0, REG_EBX, 0x00000010, VENDOR_INTEL                                , "HLE"},      /* Hardware Lock Elision */
	{ 0x00000007, 0, REG_EBX, 0x00000020, VENDOR_INTEL                                , "AVX2"},
/*	{ 0x00000007, 0, REG_EBX, 0x00000040, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x00000080, VENDOR_INTEL                                , "SMEP"},     /* Supervisor Mode Execution Protection */
	{ 0x00000007, 0, REG_EBX, 0x00000100, VENDOR_INTEL                                , "BMI2"},
	{ 0x00000007, 0, REG_EBX, 0x00000200, VENDOR_INTEL                                , "ERMS"},     /* Enhanced REP MOVSB/STOSB */
	{ 0x00000007, 0, REG_EBX, 0x00000400, VENDOR_INTEL                                , "INVPCID"},
	{ 0x00000007, 0, REG_EBX, 0x00000800, VENDOR_INTEL                                , "RTM"},      /* Restricted Transactional Memory */
	{ 0x00000007, 0, REG_EBX, 0x00001000, VENDOR_INTEL                                , "PQM"},      /* Platform Quality of Service Monitoring (PQM) */
	{ 0x00000007, 0, REG_EBX, 0x00002000, VENDOR_INTEL                                , "CSDS_DEP"}, /* FPU CS and FPU DS values deprecated */
	{ 0x00000007, 0, REG_EBX, 0x00004000, VENDOR_INTEL                                , "MPX"},      /* Memory Protection Extensions */
	{ 0x00000007, 0, REG_EBX, 0x00008000, VENDOR_INTEL                                , "PQE"},      /* Platform Quality of Service Enforcement (PQE) */
	{ 0x00000007, 0, REG_EBX, 0x00010000, VENDOR_INTEL                                , "AVX512F"},
/*	{ 0x00000007, 0, REG_EBX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x00040000, VENDOR_INTEL                                , "RDSEED"},
	{ 0x00000007, 0, REG_EBX, 0x00080000, VENDOR_INTEL                                , "ADX"},
	{ 0x00000007, 0, REG_EBX, 0x00100000, VENDOR_INTEL                                , "SMAP"},
/*	{ 0x00000007, 0, REG_EBX, 0x00200000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EBX, 0x00400000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , "CLFSHOPT"},
/*	{ 0x00000007, 0, REG_EBX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x00000007, 0, REG_EBX, 0x02000000, VENDOR_INTEL                                , "PTRACE"},   /* Intel Processor Trace */
	{ 0x00000007, 0, REG_EBX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "AVX512PF"},
	{ 0x00000007, 0, REG_EBX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , "AVX512ER"},
	{ 0x00000007, 0, REG_EBX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , "AVX512CD"},
	{ 0x00000007, 0, REG_EBX, 0x20000000, VENDOR_INTEL                                , "SHA"},
/*	{ 0x00000007, 0, REG_EBX, 0x40000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_EBX, 0x80000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */

/*	{ 0x00000007, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x00000007, 0, REG_ECX, 0x00000002, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
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
	{ 0x80000001, 0, REG_EDX, 0x00100000, VENDOR_INTEL                                , "XD"},
	{ 0x80000001, 0, REG_EDX, 0x00100000,                VENDOR_AMD                   , "NX"},
/*	{ 0x80000001, 0, REG_EDX, 0x00200000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x00400000,                VENDOR_AMD                   , "MMXEXT"},
/*	{ 0x80000001, 0, REG_EDX, 0x00800000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_EDX, 0x01000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x02000000,                VENDOR_AMD                   , "FFXSR"},
	{ 0x80000001, 0, REG_EDX, 0x04000000, VENDOR_INTEL | VENDOR_AMD                   , "P1GB"},
	{ 0x80000001, 0, REG_EDX, 0x08000000, VENDOR_INTEL | VENDOR_AMD                   , "RDTSCP"},
/*	{ 0x80000001, 0, REG_EDX, 0x10000000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_EDX, 0x20000000, VENDOR_INTEL                                , "EM64T"},
	{ 0x80000001, 0, REG_EDX, 0x20000000,                VENDOR_AMD                   , "LM"},
	{ 0x80000001, 0, REG_EDX, 0x40000000,                VENDOR_AMD                   , "3DNOWEXT"},
	{ 0x80000001, 0, REG_EDX, 0x80000000,                VENDOR_AMD                   , "3DNOW"},

	{ 0x80000001, 0, REG_ECX, 0x00000001, VENDOR_INTEL | VENDOR_AMD                   , "LAHF"},
	{ 0x80000001, 0, REG_ECX, 0x00000002,                VENDOR_AMD                   , "CL"},
	{ 0x80000001, 0, REG_ECX, 0x00000004,                VENDOR_AMD                   , "SVM"},
	{ 0x80000001, 0, REG_ECX, 0x00000008,                VENDOR_AMD                   , "EAS"},
	{ 0x80000001, 0, REG_ECX, 0x00000010,                VENDOR_AMD                   , "AMC8"},
	{ 0x80000001, 0, REG_ECX, 0x00000020,                VENDOR_AMD                   , "ABM"},
	{ 0x80000001, 0, REG_ECX, 0x00000020, VENDOR_INTEL                                , "LZCNT"},
	{ 0x80000001, 0, REG_ECX, 0x00000040,                VENDOR_AMD                   , "SSE4A"},
	{ 0x80000001, 0, REG_ECX, 0x00000080,                VENDOR_AMD                   , "MAS"},
	{ 0x80000001, 0, REG_ECX, 0x00000100,                VENDOR_AMD                   , "3DNP"},
	{ 0x80000001, 0, REG_ECX, 0x00000200,                VENDOR_AMD                   , "OSVW"},
	{ 0x80000001, 0, REG_ECX, 0x00000400,                VENDOR_AMD                   , "IBS"},
	{ 0x80000001, 0, REG_ECX, 0x00000800,                VENDOR_AMD                   , "XOP"},
	{ 0x80000001, 0, REG_ECX, 0x00001000,                VENDOR_AMD                   , "SKINIT"},
	{ 0x80000001, 0, REG_ECX, 0x00002000,                VENDOR_AMD                   , "WDT"},
/*	{ 0x80000001, 0, REG_ECX, 0x00004000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00008000,                VENDOR_AMD                   , "LWP"},
	{ 0x80000001, 0, REG_ECX, 0x00010000,                VENDOR_AMD                   , "FMA4"},
/*	{ 0x80000001, 0, REG_ECX, 0x00020000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
/*	{ 0x80000001, 0, REG_ECX, 0x00040000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00080000,                VENDOR_AMD                   , "NID"},
/*	{ 0x80000001, 0, REG_ECX, 0x00100000, VENDOR_INTEL | VENDOR_AMD                   , ""}, */   /* Reserved */
	{ 0x80000001, 0, REG_ECX, 0x00200000,                VENDOR_AMD                   , "TBM"},
	{ 0x80000001, 0, REG_ECX, 0x00400000,                VENDOR_AMD                   , "TOPO"},
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
	uint8_t count = 0;
	switch(state->last_leaf.eax) {
	case 0x00000001:
		printf("Base features:\n");

		/* We account for EAX/EBX separately with leaf 0x1, as these contain
		 * metadata other than feature flags.
		 */
		regs->eax = regs->ebx = 0;
		break;
	case 0x40000003:
		printf("Hyper-V features:\n");
		break;
	case 0x80000001:
		printf("Extended features:\n");
		break;
	}
	while (p && p->m_reg != REG_NULL) {
		uint32_t *reg = NULL;

		if (state->last_leaf.eax != p->m_level) {
			p++;
			continue;
		}
		if (state->last_leaf.ecx != p->m_index) {
			p++;
			continue;
		}

		switch (p->m_reg) {
		case REG_EAX:
			reg = &regs->eax;
			break;
		case REG_EBX:
			reg = &regs->ebx;
			break;
		case REG_ECX:
			reg = &regs->ecx;
			break;
		case REG_EDX:
			reg = &regs->edx;
			break;
		default:
			abort();
		}

		if (ignore_vendor) {
			if ((*reg & p->m_bitmask) != 0)
			{
				char feat[32], buffer[32];
				sprintf(feat, "%s (%s)", p->m_name, vendors(buffer, p->m_vendor));
				printf("  %-38s", feat);
				count++;
				if (count == 2) {
					count = 0;
					printf("\n");
				}
				*reg &= (~p->m_bitmask);
			}
		} else {
			if (((int)p->m_vendor == VENDOR_ANY || (state->vendor & p->m_vendor) != 0)
				&& (*reg & p->m_bitmask) != 0)
			{
				printf("  %-14s", p->m_name);
				count++;
				if (count == 4) {
					count = 0;
					printf("\n");
				}
				*reg &= (~p->m_bitmask);
			}
		}
		p++;
	}

	if (count != 0)
		printf("\n");

	if (regs->eax || regs->ebx || regs->ecx || regs->edx)
		printf("Unaccounted for in 0x%08x:0x%08x:\n  eax: 0x%08x ebx:0x%08x ecx:0x%08x edx:0x%08x\n",
			state->last_leaf.eax, state->last_leaf.ecx,
		    regs->eax, regs->ebx, regs->ecx, regs->edx);
}
