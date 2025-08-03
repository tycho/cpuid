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

#include "cache.h"
#include "feature.h"
#include "handlers.h"
#include "state.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

#define DECLARE_HANDLER(name) \
	static void handle_ ## name (struct cpu_regs_t *, struct cpuid_state_t *)

DECLARE_HANDLER(features);

struct x2apic_prop_t {
	uint32_t mask;
	uint8_t shift;
	uint8_t total;
	unsigned reported:1;
};

struct x2apic_infer_t {
	uint32_t sockets;
	uint32_t cores_per_socket;
	uint32_t threads_per_core;
};

struct x2apic_state_t {
	uint32_t id;
	struct x2apic_prop_t socket;
	struct x2apic_prop_t core;
	struct x2apic_prop_t thread;
	struct x2apic_infer_t infer;
};
static int probe_std_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state, struct x2apic_state_t *apic);

DECLARE_HANDLER(std_base);
DECLARE_HANDLER(std_cache);
DECLARE_HANDLER(std_psn);
DECLARE_HANDLER(std_dcp);
DECLARE_HANDLER(std_monitor);
DECLARE_HANDLER(std_power);
DECLARE_HANDLER(std_extfeat);
DECLARE_HANDLER(std_x2apic);
DECLARE_HANDLER(std_perfmon);
DECLARE_HANDLER(std_ext_state);
DECLARE_HANDLER(std_qos_monitor);
//DECLARE_HANDLER(std_qos_enforce);
//DECLARE_HANDLER(std_sgx);
DECLARE_HANDLER(std_trace);
DECLARE_HANDLER(std_tsc);
DECLARE_HANDLER(std_cpufreq);
//DECLARE_HANDLER(std_soc);
DECLARE_HANDLER(std_tlb);
//DECLARE_HANDLER(std_keylocker);
//DECLARE_HANDLER(std_hybrid);
//DECLARE_HANDLER(std_pconfig);
//DECLARE_HANDLER(std_tile);
//DECLARE_HANDLER(std_tmul);
//DECLARE_HANDLER(std_x2apic_v2);
//DECLARE_HANDLER(std_hreset);

DECLARE_HANDLER(ext_base);
DECLARE_HANDLER(ext_pname);
DECLARE_HANDLER(ext_amdl1cachefeat);
DECLARE_HANDLER(ext_l2cachefeat);
DECLARE_HANDLER(ext_0008);
DECLARE_HANDLER(ext_svm);
DECLARE_HANDLER(ext_amd_1g_tlb);
DECLARE_HANDLER(ext_perf_opt_feat);
DECLARE_HANDLER(ext_ibs_feat);
DECLARE_HANDLER(ext_cacheprop);
DECLARE_HANDLER(ext_extapic);
//DECLARE_HANDLER(ext_amd_encryption);
//DECLARE_HANDLER(ext_amd_mem_qos);

DECLARE_HANDLER(vmm_base);
DECLARE_HANDLER(vmm_leaf01);
DECLARE_HANDLER(vmm_leaf02);
DECLARE_HANDLER(vmm_leaf03);
DECLARE_HANDLER(vmm_leaf04);
DECLARE_HANDLER(vmm_leaf05);
DECLARE_HANDLER(vmm_leaf06);
DECLARE_HANDLER(hyperv_leaf07);
DECLARE_HANDLER(hyperv_leaf08);
DECLARE_HANDLER(hyperv_leaf09);
DECLARE_HANDLER(hyperv_leaf0A);
DECLARE_HANDLER(vmware_leaf10);

DECLARE_HANDLER(tmta_base);
DECLARE_HANDLER(tmta_cmsinfo);

DECLARE_HANDLER(centaur_base);

DECLARE_HANDLER(dump_base);
DECLARE_HANDLER(dump_until_eax);
DECLARE_HANDLER(dump_vmm_leaf03);
DECLARE_HANDLER(dump_std_04);
DECLARE_HANDLER(dump_x2apic);
DECLARE_HANDLER(dump_std_0D);
DECLARE_HANDLER(dump_std_0F);
DECLARE_HANDLER(dump_std_10);
DECLARE_HANDLER(dump_std_12);
DECLARE_HANDLER(dump_std_1B);
DECLARE_HANDLER(dump_ext_1D);
DECLARE_HANDLER(dump_ext_20);

const struct cpuid_leaf_handler_index_t dump_handlers[] =
{
	/* Standard levels */
	{0x00000000, handle_dump_base},
	{0x00000004, handle_dump_std_04},
	{0x00000007, handle_dump_until_eax},
	{0x0000000B, handle_dump_x2apic},
	{0x0000000D, handle_dump_std_0D},
	{0x0000000F, handle_dump_std_0F},
	{0x00000010, handle_dump_std_10},
	{0x00000012, handle_dump_std_12},
	{0x00000014, handle_dump_until_eax},
	{0x00000017, handle_dump_until_eax},
	{0x00000018, handle_dump_until_eax},
	{0x0000001B, handle_dump_std_1B},
	{0x0000001D, handle_dump_until_eax},
	{0x0000001F, handle_dump_x2apic},
	{0x00000020, handle_dump_until_eax},
	{0x00000023, handle_dump_until_eax},

	/* Hypervisor levels */
	{0x40000000, handle_dump_base},
	{0x40000003, handle_dump_vmm_leaf03},

	/* Extended levels */
	{0x80000000, handle_dump_base},
	{0x8000001D, handle_dump_ext_1D},
	{0x80000020, handle_dump_ext_20},

	/* Transmeta leaves */
	{0x80860000, handle_dump_base},

	/* Centaur leaves */
	{0xc0000000, handle_dump_base},

	{0, 0}
};

const struct cpuid_leaf_handler_index_t decode_handlers[] =
{
	/* Standard levels */
	{0x00000000, handle_std_base},
	{0x00000001, handle_features},
	{0x00000002, handle_std_cache},
	{0x00000003, handle_std_psn},
	{0x00000004, handle_std_dcp},
	{0x00000005, handle_std_monitor},
	{0x00000006, handle_std_power},
	{0x00000007, handle_std_extfeat},
	{0x0000000A, handle_std_perfmon},
	{0x0000000B, handle_std_x2apic},
	{0x0000000D, handle_std_ext_state},
	{0x0000000F, handle_std_qos_monitor},
	//{0x00000010, handle_std_qos_enforce},
	//{0x00000012, handle_std_sgx},
	{0x00000014, handle_std_trace},
	{0x00000015, handle_std_tsc},
	{0x00000016, handle_std_cpufreq},
	{0x00000018, handle_std_tlb},

	/* TODO, when I have hardware that I can develop/test these on. */
	//{0x00000017, handle_std_soc},
	//{0x00000019, handle_std_keylocker},
	//{0x0000001a, handle_std_hybrid},
	//{0x0000001b, handle_std_pconfig},
	//{0x0000001d, handle_std_tile},
	//{0x0000001e, handle_std_tmul},
	//{0x0000001f, handle_std_x2apic_v2},
	//{0x00000020, handle_std_hreset},

	/* Hypervisor levels */
	{0x40000000, handle_vmm_base},
	{0x40000001, handle_vmm_leaf01},
	{0x40000002, handle_vmm_leaf02},
	{0x40000003, handle_vmm_leaf03},
	{0x40000004, handle_vmm_leaf04},
	{0x40000005, handle_vmm_leaf05},
	{0x40000006, handle_vmm_leaf06},
	{0x40000007, handle_hyperv_leaf07},
	{0x40000008, handle_hyperv_leaf08},
	{0x40000009, handle_hyperv_leaf09},
	{0x4000000A, handle_hyperv_leaf0A},
	{0x40000010, handle_vmware_leaf10},

	/* Extended levels */
	{0x80000000, handle_ext_base},
	{0x80000001, handle_features},
	{0x80000002, handle_ext_pname},
	{0x80000003, handle_ext_pname},
	{0x80000004, handle_ext_pname},
	{0x80000005, handle_ext_amdl1cachefeat},
	{0x80000006, handle_ext_l2cachefeat},
	{0x80000007, handle_features},
	{0x80000008, handle_ext_0008},
	{0x8000000A, handle_ext_svm},
	{0x80000019, handle_ext_amd_1g_tlb},
	{0x8000001A, handle_ext_perf_opt_feat},
	{0x8000001B, handle_ext_ibs_feat},
	{0x8000001D, handle_ext_cacheprop},
	{0x8000001E, handle_ext_extapic},
	//{0x8000001F, handle_ext_amd_encryption},
	//{0x80000020, handle_ext_amd_mem_qos},

	/* Transmeta levels */
	{0x80860000, handle_tmta_base},
	{0x80860003, handle_tmta_cmsinfo},
	{0x80860004, handle_tmta_cmsinfo},
	{0x80860005, handle_tmta_cmsinfo},
	{0x80860006, handle_tmta_cmsinfo},

	/* Centaur levels */
	{0xc0000000, handle_centaur_base},
	{0xc0000001, handle_features},

	{0, 0}
};

/* EAX = 0000 0000 | EAX = 4000 0000 | EAX = 8000 0000 */
static void handle_dump_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->curmax = regs->eax;
	state->cpuid_print(regs, state, FALSE);
}

typedef struct _vendor_map_t {
	const char *name;
	int id;
} vendor_map_t, *pvendor_map_t;

vendor_map_t vendors[] = {
	{ "GenuineIntel", VENDOR_INTEL },
	{ "GenuineIotel", VENDOR_INTEL }, /* There are a few of these out there, oddly... */
	{ "AuthenticAMD", VENDOR_AMD },
	{ "GenuineTMx86", VENDOR_TRANSMETA },
	{ "CyrixInstead", VENDOR_CYRIX },
	{ "HygonGenuine", VENDOR_HYGON },
	{ "CentaurHauls", VENDOR_CENTAUR },
	{ NULL, VENDOR_UNKNOWN },
};


int vendor_id(const char *name)
{
	pvendor_map_t pvendor = vendors;

	while (pvendor->name) {
		if (strcmp(name, pvendor->name) == 0)
			break;
		pvendor++;
	}

	return pvendor->id;
}

const char *vendor_name(int vendor_id)
{
	pvendor_map_t pvendor = vendors;
	while (pvendor->name) {
		if (pvendor->id == vendor_id)
			break;
		pvendor++;
	}
	return pvendor->name;
}

/* EAX = 0000 0000 */
static void handle_std_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13] ALIGNED(4);
	size_t i;

	state->curmax = regs->eax;

	printf("Maximum basic CPUID leaf: 0x%08x\n\n", state->curmax);

	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->edx;
	*(uint32_t *)(&buf[8]) = regs->ecx;
	buf[12] = 0;

	for (i = 0; i < sizeof(buf); i++) {
		/* End of vendor string */
		if (buf[i] == 0)
			break;

		/* Character outside printable range */
		if (buf[i] < 0x20 || buf[i] > 0x7E)
			buf[i] = '.';
	}

	buf[12] = 0;

	printf("CPU vendor string: '%s'", buf);
	if (state->vendor == VENDOR_UNKNOWN) {
		state->vendor = vendor_id(buf);

		/* Hygon is an AMD EPYC clone, let's allow decoding AMD-specific CPUID leaves */
		if (state->vendor == VENDOR_HYGON)
			state->vendor |= VENDOR_AMD;
	} else if (state->vendor_override) {
		printf(" (overridden as '%s')", vendor_name(state->vendor));
	}
	printf("\n\n");

	if (state->vendor == VENDOR_UNKNOWN) {
		/* Weird CPU, ignore vendor string for purpose of feature flag
		 * identification
		 */
		state->ignore_vendor = 1;
	}

	/* Try to probe topology early, to set up state->logical_in_socket */
	{
		struct x2apic_state_t x2apic;
		if (probe_std_x2apic(regs, state, &x2apic))
			state->logical_in_socket = state->cpu_logical_count;
	}
}

static const struct amd_package_match_t
{
	uint32_t family;
	uint32_t extmodel;
	uint32_t package_id;
	const char *name;
} amd_package_match [] = {
	{ 0x10, 0xFFFFFFFF, 0, "F" },
	{ 0x10, 0xFFFFFFFF, 1, "AM" },
	{ 0x10, 0xFFFFFFFF, 2, "S1" },
	{ 0x10, 0xFFFFFFFF, 3, "G34" },
	{ 0x10, 0xFFFFFFFF, 4, "ASB2" },
	{ 0x10, 0xFFFFFFFF, 5, "C32" },
	{ 0x11, 0xFFFFFFFF, 2, "S1g2" },
	{ 0x12, 0xFFFFFFFF, 1, "FS1 (µPGA)" },
	{ 0x12, 0xFFFFFFFF, 2, "FM1 (PGA)" },
	{ 0x14, 0xFFFFFFFF, 0, "FT1 (BGA)" },
	{ 0x14, 0xFFFFFFFF,15, "FT1 (BGA)" },
	{ 0x15, 0, 1, "AM3" },
	{ 0x15, 0, 3, "G34" },
	{ 0x15, 0, 5, "C32" },
	{ 0x15, 1, 0, "FP2 (BGA)" },
	{ 0x15, 1, 1, "FS1r2 (µPGA)" },
	{ 0x15, 1, 2, "FM2 (PGA)" },
	{ 0x15, 3, 0, "FP3 (BGA)" },
	{ 0x15, 3, 1, "FM2r2 (µPGA)" },
	{ 0x15, 6, 0, "FP4 (BGA)" },
	{ 0x15, 6, 2, "AM4 (µPGA)" },
	{ 0x15, 6, 3, "FM2r2 (µPGA)" },
	{ 0x15, 7, 0, "FP4 (BGA)" },
	{ 0x15, 7, 2, "AM4 (µPGA)" },
	{ 0x15, 7, 4, "FT4 (BGA)" },
	{ 0x16, 0, 0, "FT3 (BGA)" },
	{ 0x16, 0, 1, "FS1b" },
	{ 0x16, 3, 0, "FT3b (BGA)" },
	{ 0x16, 3, 3, "FP4"},
	{ 0x17, 0, 1, "SP4"},
	{ 0x17, 0, 2, "AM4"},
	{ 0x17, 0, 3, "SP4r2"},
	{ 0x17, 0, 4, "SP3"},
	{ 0x17, 0, 7, "SP3r2"},
	{ 0x17, 1, 0, "FP5"},
	{ 0x17, 2, 0, "FP5"},
	{ 0x17, 3, 4, "SP3"},
	{ 0x17, 3, 7, "TRX4"},
	{ 0x17, 1, 2, "AM4"},
	{ 0x17, 6, 0, "FP6"},
	{ 0x17, 7, 2, "AM4"},
	{ 0x17, 8, 0, "FP5"},
	{ 0x19, 0xFFFFFFFF, 0, "FP6 (µPGA)" },
	{ 0x19, 0xFFFFFFFF, 2, "AM4"},

	{ 0, 0, 0, NULL },
};

/* EAX = 8000 0001 | EAX = 0000 0001 */
static void handle_features(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->last_leaf.eax == 0x00000001) {
		struct std1_ebx_t
		{
			uint8_t brandid;
			uint8_t clflushsz;
			uint8_t logicalcount;
			uint8_t localapicid;
		};
		uint32_t model;
		struct std1_ebx_t *ebx = (struct std1_ebx_t *)&regs->ebx;
		state->sig_int = regs->eax;

		/* Model is calculated differently on Intel/AMD. */
		model = state->sig.model;
		if (state->vendor & VENDOR_INTEL) {
			model += ((state->sig.family == 0xf || state->sig.family == 0x6) ? state->sig.extmodel << 4 : 0);
		} else if (state->vendor & VENDOR_AMD) {
			model += (state->sig.family == 0xf ? state->sig.extmodel << 4 : 0);
		}

		/* Store the derived values */
		state->family = state->sig.family + state->sig.extfamily;
		state->model = model;

		if (regs->ecx & (1U << 31U)) {
			state->vendor |= VENDOR_HV_GENERIC;
		}

		printf("Signature:  0x%08x\n"
		       "  Family:   0x%02x (%d)\n"
		       "  Model:    0x%02x (%d)\n"
		       "  Stepping: 0x%02x (%d)\n\n",
		       state->sig_int,
		       state->sig.family + state->sig.extfamily,
		       state->sig.family + state->sig.extfamily,
		       model,
		       model,
		       state->sig.stepping,
		       state->sig.stepping);
		printf("Local APIC: %d\n"
		       "Maximum number of APIC IDs per package: %d\n"
		       "CLFLUSH size: %d\n"
		       "Brand ID: %d\n\n",
		       ebx->localapicid,
		       ebx->logicalcount,
		       ebx->clflushsz << 3,
		       ebx->brandid);
	} else if (state->last_leaf.eax == 0x80000001 && state->family >= 0x10) {
		uint32_t package_id = (regs->ebx >> 28) & 0xf;
		const struct amd_package_match_t *match;
		for (match = amd_package_match; match->name != NULL; match++) {
			BOOL matches = TRUE;
			matches &= (match->family == state->family);
			if (match->extmodel != 0xFFFFFFFF)
				matches &= (match->extmodel == state->sig.extmodel);
			matches &= (match->package_id == package_id);
			if (matches)
				break;
		}
		printf("CPU Socket: %s", match->name ? match->name : "Unknown");
		if (!match->name)
			printf(" (%02x:%02x:%02x)",
					state->family,
					state->sig.extmodel,
					package_id);
		printf("\n\n");
	}
	if (print_features(regs, state))
		printf("\n");
}

/* EAX = 0000 0002 */
static void handle_std_cache(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint8_t i, m = regs->eax & 0xFF;
	struct cpu_regs_t *rvec = NULL;

	/* I don't think this ever happens, but just in case... */
	if (m < 1)
		return;

	rvec = (struct cpu_regs_t *)malloc(sizeof(struct cpu_regs_t) * m);
	if (!rvec)
		return;

	/* We have the first result already, copy it over. */
	memcpy(&rvec[0], regs, sizeof(struct cpu_regs_t));

	/* Now we can reuse 'regs' as an offset. */
	regs = &rvec[1];
	for (i = 1; i < m; i++) {
		ZERO_REGS(regs);
		regs->eax = 2;
		state->cpuid_call(regs, state);
		regs++;
	}

	/* Printout time. */
	printf("Cache descriptors:\n");
	regs = rvec;
	for (i = 0; i < m; i++) {
		print_intel_caches(regs, &state->sig);
		regs++;
	}

	free(rvec);
}

/* EAX = 0000 0003 */
static void handle_std_psn(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if ((state->vendor & (VENDOR_INTEL | VENDOR_TRANSMETA)) == 0)
		return;
	ZERO_REGS(regs);
	regs->eax = 0x01;
	state->cpuid_call(regs, state);
	if ((regs->edx & 0x00040000) == 0) {
		printf("Processor serial number: disabled (or not supported)\n\n");
		return;
	}
	if (state->vendor & VENDOR_TRANSMETA) {
		ZERO_REGS(regs);
		regs->eax = 0x03;
		state->cpuid_call(regs, state);
		printf("Processor serial number (Transmeta encoding): %08X-%08X-%08X-%08X\n\n",
		       regs->eax, regs->ebx, regs->ecx, regs->edx);
	}
	if (state->vendor & VENDOR_INTEL) {
		uint32_t ser_eax = regs->eax;
		ZERO_REGS(regs);
		regs->eax = 0x03;
		state->cpuid_call(regs, state);
		printf("Processor serial number (Intel encoding): %04X-%04X-%04X-%04X-%04X-%04X\n\n",
		       ser_eax >> 16, ser_eax & 0xFFFF,
		       regs->edx >> 16, regs->edx & 0xFFFF,
		       regs->ecx >> 16, regs->ecx & 0xFFFF);
	}
}

/* EAX = 0000 0004 */
static void handle_std_dcp(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_cache04_t {
		unsigned type:5;
		unsigned level:3;
		unsigned self_initializing:1;
		unsigned fully_associative:1;
		unsigned reserved:4;
		unsigned max_threads_sharing:12; /* +1 encoded */
		unsigned apics_reserved:6; /* +1 encoded */
	};
	struct ebx_cache04_t {
		unsigned line_size:12; /* +1 encoded */
		unsigned partitions:10; /* +1 encoded */
		unsigned assoc:10; /* +1 encoded */
	};
	uint32_t i = 0;

	if ((state->vendor & VENDOR_INTEL) == 0)
		return;

	printf("Deterministic Cache Parameters:\n");
	if (sizeof(struct eax_cache04_t) != 4 || sizeof(struct ebx_cache04_t) != 4) {
		printf("  WARNING: The code appears to have been incorrectly compiled.\n"
		       "           Expect wildly inaccurate output for this section.\n");
	}

	while (1) {
		struct cache_desc_t desc;
		char desc_str[512];
		struct eax_cache04_t *eax;
		struct ebx_cache04_t *ebx;
		uint32_t cacheSize;
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		state->cpuid_call(regs, state);

		if ((regs->eax & 0x1f) == 0)
			break;

		eax = (struct eax_cache04_t *)&regs->eax;
		ebx = (struct ebx_cache04_t *)&regs->ebx;

		/* Cache size calculated in bytes. */
		cacheSize = (ebx->assoc + 1) *
			(ebx->partitions + 1) *
			(ebx->line_size + 1) *
			(regs->ecx + 1);

		/* Convert to kilobytes. */
		cacheSize /= 1024;

		desc.level = L0 + eax->level;
		desc.type = DATA + eax->type - 1;
		desc.size = cacheSize;
		desc.attrs = (eax->self_initializing ? SELF_INIT : 0) |
		             ((regs->edx & 0x01) ? WBINVD_NOT_INCLUSIVE : 0) |
		             ((regs->edx & 0x02) ? INCLUSIVE : 0) |
		             ((regs->edx & 0x04) ? CPLX_INDEX : 0);
		desc.assoc = eax->fully_associative ? 0xff : ebx->assoc + 1;
		desc.linesize = ebx->line_size + 1;
		desc.partitions = ebx->partitions + 1;
		desc.max_threads_sharing = eax->max_threads_sharing + 1;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));

		/* This is the official termination condition for this leaf. */
		if (!(regs->eax & 0xF))
			break;

		i++;
	}
}

/* EAX = 0000 0004 */
static void handle_dump_std_04(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		if (!(regs->eax & 0xF))
			break;
		i++;
	}
}

/* EAX = 0000 0005 */
static void handle_std_monitor(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	/* MONITOR/MWAIT leaf */
	struct eax_monitor_t {
		unsigned smallest_line:16;
		unsigned reserved:16;
	};
	struct ebx_monitor_t {
		unsigned largest_line:16;
		unsigned reserved:16;
	};
	struct ecx_monitor_t {
		unsigned enumeration:1;
		unsigned interrupts_as_break:1;
		unsigned reserved:20;
	};
	struct edx_monitor_t {
		unsigned c:32;
	};
	unsigned int i;
	struct eax_monitor_t *eax = (struct eax_monitor_t *)&regs->eax;
	struct ebx_monitor_t *ebx = (struct ebx_monitor_t *)&regs->ebx;
	struct ecx_monitor_t *ecx = (struct ecx_monitor_t *)&regs->ecx;
	struct edx_monitor_t *edx = (struct edx_monitor_t *)&regs->edx;
	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;
	if (!(regs->eax || regs->ebx))
		return;
	printf("MONITOR/MWAIT features:\n");
	printf("  Smallest monitor-line size: %d bytes\n", eax->smallest_line);
	printf("  Largest monitor-line size: %d bytes\n", ebx->largest_line);
	if (!ecx->enumeration)
		goto no_enumeration;
	if (ecx->interrupts_as_break)
		printf("  Interrupts as break-event for MWAIT, even when interrupts off\n");
	if (state->vendor & VENDOR_INTEL) {
		for (i = 0; i < 8; i++) {
			uint8_t states = (edx->c >> (i * 4)) & 0xF;
			if (states)
				printf("  C%d sub C-states supported by MWAIT: %d\n", i, states);
		}
	}
no_enumeration:
	printf("\n");
}

/* EAX = 0000 0006 */
static void handle_std_power(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct ebx_power_t {
		unsigned dts_thresholds:4;
		unsigned reserved:28;
	};
	struct ebx_power_t *ebx = (struct ebx_power_t *)&regs->ebx;

	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;

	/* If we don't have anything to print, skip. */
	if (!(regs->eax || regs->ebx || regs->ecx))
		return;

	printf("Intel Thermal and Power Management Features:\n");
	print_features(regs, state);
	if (ebx->dts_thresholds)
		printf("  Interrupt thresholds in DTS: %d\n", ebx->dts_thresholds);
	printf("\n");
}

/* EAX = 0000 0007 */
static void handle_std_extfeat(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0, m = regs->eax;
	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;
	if (!(regs->eax || regs->ebx || regs->ecx || regs->edx))
		return;
	while (i <= m) {
		ZERO_REGS(regs);
		regs->eax = 0x7;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		print_features(regs, state);
		i++;
		printf("\n");
	}
}

/* EAX = 0000 0007 and EAX = 0000 0017 */
static void handle_dump_until_eax(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t eax = state->last_leaf.eax;
	uint32_t max_ecx = regs->eax;
	uint32_t i = 0;
	while (i <= max_ecx) {
		ZERO_REGS(regs);
		regs->eax = eax;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

/* EAX = 0000 000A */
static void handle_std_perfmon(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct perfmon_eax_t {
		unsigned version:8;
		unsigned pmc_per_logical:8;
		unsigned bit_width_pmc:8;
		unsigned ebx_length:8;
	};
	struct perfmon_ebx_feat_t {
		unsigned int mask;
		const char *name;
	} features[] = {
		{0x00000001, "Core cycles"},
		{0x00000002, "Instructions retired"},
		{0x00000004, "Reference cycles"},
		{0x00000008, "Last-level cache reference"},
		{0x00000010, "Last-level cache miss"},
		{0x00000020, "Branches retired"},
		{0x00000040, "Branches mispredicted"},
		{0x00000080, "Top-down slots event"},
		{0x00000000, NULL}
	};
	struct perfmon_edx_t {
		unsigned count_ff:5;
		unsigned bit_width_ff:8;
		unsigned reserved:2;
		unsigned anythread_deprecated:1;
		unsigned reserved2:16;
	};

	struct perfmon_eax_t *eax = (struct perfmon_eax_t *)&regs->eax;
	struct perfmon_edx_t *edx = (struct perfmon_edx_t *)&regs->edx;
	struct perfmon_ebx_feat_t *feat;

	if ((state->vendor & VENDOR_INTEL) == 0)
		return;

	if (!eax->version)
		return;

	printf("Architectural Performance Monitoring\n");
	printf("  Version: %u\n", eax->version);
	printf("  Counters per logical processor: %u\n", eax->pmc_per_logical);
	printf("  Counter bit width: %u\n", eax->bit_width_pmc);
	printf("  Number of contiguous fixed-function counters: %u\n", edx->count_ff);
	printf("  Bit width of fixed-function counters: %u\n", edx->bit_width_ff);
	if (edx->anythread_deprecated)
		printf("  AnyThread deprecated\n");

	printf("  Supported performance counters:\n");
	for (feat = features; feat->name; feat++)
	{
		if (feat->mask > (1u << eax->ebx_length))
			break;

		/* 1 == unavailable for some bizarre reason. */
		if ((regs->ebx & feat->mask) == 0)
			printf("    %s\n", feat->name);
	}
	printf("\n");
}

/* EAX = 0000 000B */
static int probe_std_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state, struct x2apic_state_t *x2apic)
{
	uint32_t i;
	uint32_t total_logical = state->thread_count(state);

	/* Check if x2APIC is supported. Early exit if not. */
	ZERO_REGS(regs);
	regs->eax = 0xb;
	state->cpuid_call(regs, state);
	if (!regs->eax && !regs->ebx)
		return 1;

	memset(x2apic, 0, sizeof(struct x2apic_state_t));
	x2apic->socket.reported = 1;
	x2apic->socket.mask = -1;

	/* Inferrence */
	for (i = 0;; i++) {
		uint32_t level, shift;
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (!(regs->eax || regs->ebx || regs->ecx || regs->edx))
			break;
		level = (regs->ecx >> 8) & 0xff;
		shift = regs->eax & 0x1f;
		if (level > 0)
			x2apic->id = regs->edx;
		else
			break;
		switch (level) {
		case 1: /* Thread level */
			x2apic->thread.total = regs->ebx & 0xffff;
			x2apic->thread.shift = shift;
			x2apic->thread.mask = ~((unsigned)(-1) << shift);
			x2apic->thread.reported = 1;
			break;
		case 2: /* Core level */
			x2apic->core.total = regs->ebx & 0xffff;
			x2apic->core.shift = shift;
			x2apic->core.mask = ~((unsigned)(-1) << shift);
			x2apic->core.reported = 1;
			x2apic->socket.shift = x2apic->core.shift;
			x2apic->socket.mask = (-1) ^ x2apic->core.mask;
			break;
		}
		if (!(regs->eax || regs->ebx))
			break;
	}
	if (x2apic->thread.reported && x2apic->core.reported) {
		x2apic->core.mask = x2apic->core.mask ^ x2apic->thread.mask;
	} else if (!x2apic->core.reported && x2apic->thread.reported) {
		x2apic->core.mask = 0;
		x2apic->core.total = 1;
		x2apic->socket.shift = x2apic->thread.shift;
		x2apic->socket.mask = (-1) ^ x2apic->thread.mask;
	} else {
		return 1;
	}

	/* XXX: This is a totally non-standard way to determine the shift width,
	 *      but the official method doesn't seem to work. Will troubleshoot
	 *      more later on.
	 */
	x2apic->socket.shift = count_trailing_zero_bits(x2apic->socket.mask);
	x2apic->core.shift = count_trailing_zero_bits(x2apic->core.mask);
	x2apic->thread.shift = count_trailing_zero_bits(x2apic->thread.mask);

	/*
	printf("  Socket mask: 0x%08x, shift: %d\n", x2apic->socket.mask, x2apic->socket.shift);
	printf("  Core mask: 0x%08x, shift: %d\n", x2apic->core.mask, x2apic->core.shift);
	printf("  Thread mask: 0x%08x, shift: %d\n", x2apic->thread.mask, x2apic->thread.shift);
	*/

	if (!x2apic->core.total || !x2apic->thread.total) {
		/* Something went wrong here, if we don't bomb out now, we'll end up
		 * dividing by zero.
		 */
		return 1;
	}

	if (x2apic->core.total >  x2apic->thread.total)
		x2apic->core.total /= x2apic->thread.total;

	x2apic->infer.sockets = total_logical / x2apic->core.total * x2apic->thread.total;
	x2apic->infer.cores_per_socket = x2apic->core.total;
	x2apic->infer.threads_per_core = x2apic->thread.total;

	return 0;
}

static inline int x2apic_idx_mask(uint32_t id, struct x2apic_prop_t *prop)
{
	if (prop->shift >= 32)
		return 0;
	return (id & prop->mask) >> prop->shift;
}

/* EAX = 0000 000B */
static void handle_std_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t total_logical = state->thread_count(state);
	struct x2apic_state_t x2apic;

	if (probe_std_x2apic(regs, state, &x2apic))
		return;

	printf("x2APIC Processor Topology:\n");
	printf("  Inferred information:\n");
	printf("    Logical total:       %u%s\n", total_logical, (total_logical >= x2apic.infer.cores_per_socket * x2apic.infer.threads_per_core) ? "" : " (?)");
	printf("    Logical per socket:  %u\n",   x2apic.infer.cores_per_socket * x2apic.infer.threads_per_core);
	printf("    Cores per socket:    %u\n",   x2apic.infer.cores_per_socket);
	printf("    Threads per core:    %u\n\n", x2apic.infer.threads_per_core);

	printf("  x2APIC ID %d (socket %d, core %d, thread %d)\n\n",
	       x2apic.id,
	       x2apic_idx_mask(x2apic.id, &x2apic.socket),
	       x2apic_idx_mask(x2apic.id, &x2apic.core),
	       x2apic_idx_mask(x2apic.id, &x2apic.thread));
}

/* EAX = 0000 000B and EAX = 0000 001F */
static void handle_dump_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0;
	uint32_t eax = state->last_leaf.eax;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = eax;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (i > 0 && !(regs->eax || regs->ebx))
			break;
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

static const char *xsave_leaf_name(uint32_t bit)
{
	const char *bits[] = {
		"Legacy x87",
		"128-bit SSE",
		"256-bit AVX YMM_Hi128",
		"MPX bound registers",
		"MPX bound configuration",
		"512-bit AVX OpMask",
		"512-bit AVX ZMM_Hi256",
		"512-bit AVX ZMM_Hi16",
		"IA32_XSS",
		"Protected keys",
		NULL,
		NULL,
		NULL, /* IA32_XSS */
		NULL, /* IA32_XSS */
		NULL,
		NULL, /* IA32_XSS */
		"XTILECFG",
		"XTILEDATA",

	};
	if (bit < NELEM(bits))
		return bits[bit];
	return NULL;
}

static const char *xsave_feature_name(uint32_t bit)
{
	const char *bits[] = {
		"XSAVEOPT",
		"XSAVEC and compacted XRSTOR",
		"XGETBV with ECX=1",
		"XSAVES/XRSTORS and IA32_XSS",
		"Extended feature disable (XFD)",
	};
	if (bit < NELEM(bits))
		return bits[bit];
	return NULL;
}

/* EAX = 0000 000D */
static void handle_std_ext_state(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	int i, j, max = 0;

	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;
	if (!regs->eax)
		return;

	printf("Extended State Enumeration\n");

	for (i = 0; i <= max; i++) {

		ZERO_REGS(regs);
		regs->eax = 0xd;
		regs->ecx = i;
		state->cpuid_call(regs, state);

		if (i > 1) {
			const char *name = xsave_leaf_name(i);
			if (!name || !regs->eax)
				continue;
			printf("  Extended state for %s requires %d bytes, offset %d\n",
				name, regs->eax, regs->ebx);
		} else if (i == 1) {
			if (!regs->eax)
				continue;

			printf("  Size of XSAVE area containing all enabled states: %d\n", regs->ebx);

			printf("  Features available:\n");
			for (j = 0; j < 32; j++) {
				const char *name;
				if (!(regs->eax & (1U << j)))
					continue;
				name = xsave_feature_name(j);
				if (!name)
					name = "Unknown";
				printf("    %d - %s\n", j, name);
			}
			printf("\n");
		} else if (i == 0) {
			if (!regs->eax)
				break;

			printf("  Valid bit fields for lower 32 bits of XCR0:\n");
			for (j = 0; j < 32; j++) {
				const char *name;
				if (!(regs->eax & (1U << j)))
					continue;
				name = xsave_leaf_name(j);
				if (!name)
					name = "Unknown";
				printf("    %d - %s\n", j, name);
			}
			printf("\n");

			printf("  Valid bit fields for upper 32-bits of XCR0:\n");
			printf("    0x%08X\n", regs->edx);

			printf("\n");

			printf("  Maximum size required for all enabled features:   %3d bytes\n\n",
				regs->ebx);

			printf("  Maximum size required for all supported features: %3d bytes\n",
				regs->ecx);

			max = popcnt(regs->eax) + popcnt(regs->edx) - 1;
			printf("\n");
		}
	}
	if (max > 1)
		printf("\n");
}

/* EAX = 0000 000D */
static void handle_dump_std_0D(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xd;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (i > 1 && !(regs->eax || regs->ebx || regs->ecx || regs->edx))
			break;
		else if (i == 0 && !regs->eax)
			break;
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

/* EAX = 0000 000F */
static void handle_dump_std_0F(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0, max;
	/* note subtle difference in register used here vs leaf 0x10 */
	max = ((regs->edx & 0x2) != 0) ? 1 : 0;
	for (i = 0; i <= max; i++) {
		ZERO_REGS(regs);
		regs->eax = 0xf;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
	}
}

/* EAX = 0000 000F */
static void handle_std_qos_monitor(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct edx_qos_feature {
		unsigned int mask;
		const char *name;
	} features[] = {
		{0x00000002, "L3 cache QoS monitoring"},
		{0x00000000, NULL}
	};
	struct edx_qos_feature *feat;
	uint32_t unaccounted;

	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;

	if (!regs->edx)
		return;

	printf("Platform Quality-of-Service Monitoring\n");

	printf("  Features supported:\n");

	unaccounted = 0;
	for (feat = features; feat->mask; feat++) {
		unaccounted |= feat->mask;
		if (regs->edx & feat->mask) {
			printf("    %s\n", feat->name);
		}
	}
	unaccounted = (regs->edx & ~unaccounted);
	if (unaccounted) {
		printf("  Unaccounted feature bits: 0x%08x\n", unaccounted);
	}
	printf("\n");

	printf("  Maximum range of RMID within this physical processor: %u\n\n", regs->ebx + 1);

	/* Subleaf index 1 = L3 QoS */
	if (regs->edx & 0x2) {
		struct edx_l3qos_feature {
			unsigned int mask;
			const char *name;
		} l3qos_features[] = {
			{0x00000001, "L3 occupancy"},
			{0x00000002, "L3 total external bandwidth"},
			{0x00000004, "L3 local external bandwidth"},
			{0x00000000, NULL}
		};
		struct edx_l3qos_feature *l3qos_feat;
		ZERO_REGS(regs);
		regs->eax = 0x0F;
		regs->ecx = 1;
		state->cpuid_call(regs, state);

		printf("  L3 Cache QoS Monitoring Capabilities\n");

		printf("    Monitoring Features:\n");
		unaccounted = 0;
		for (l3qos_feat = l3qos_features; l3qos_feat->mask; l3qos_feat++) {
			unaccounted |= l3qos_feat->mask;
			if (regs->edx & l3qos_feat->mask) {
				printf("      %s\n", l3qos_feat->name);
			}
		}
		unaccounted = (regs->edx & ~unaccounted);
		if (unaccounted) {
			printf("    Unaccounted feature bits: 0x%08x\n", unaccounted);
		}
		printf("    Conversion factor from QM_CTR to occupancy metric (bytes): %u\n", regs->ebx);
		printf("    Maximum range of RMID within this resource type: %u\n", regs->ecx + 1);

	}

	printf("\n");
}

/* EAX = 0000 0010 */
static void handle_dump_std_10(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0, max;
	/* note subtle difference in register used here vs leaf 0x0f */
	max = ((regs->ebx & 0x2) != 0) ? 1 : 0;
	for (i = 0; i <= max; i++) {
		ZERO_REGS(regs);
		regs->eax = 0x10;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
	}
}

/* EAX = 0000 0012 */
static void handle_dump_std_12(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 1;

	/* Always print EAX=0x12, ECX=0 response, even if we don't support SGX. */
	state->cpuid_print(regs, state, TRUE);

	/* First check if SGX is supported. */
	ZERO_REGS(regs);
	regs->eax = 0x07;
	state->cpuid_call(regs, state);
	if ((regs->ebx & 0x00000004) == 0)
		return;

	/* SGX supported, enumerate ECX=1 through ECX=N */
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0x12;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (i > 1 && (regs->eax & 0xf) == 0)
			break;
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

/* EAX = 0000 0014 */
static void handle_std_trace(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if ((state->vendor & (VENDOR_INTEL)) == 0)
		return;

	if (!regs->eax)
		return;

	printf("Processor Trace Enumeration\n");

	print_features(regs, state);
	printf("\n");

	ZERO_REGS(regs);
	regs->eax = 0x14;
	regs->ecx = 1;
	state->cpuid_call(regs, state);

	printf("  Number of configurable address ranges for filtering: %u\n", regs->eax & 0x7);
	printf("  Supported MTC period encodings: 0x%04x\n", (regs->eax >> 16) & 0xffff);

	printf("  Supported cycle threshold value encodings: 0x%04x\n", regs->ebx & 0xffff);
	printf("  Supported configurable PSB frequency encodings: 0x%04x\n", (regs->ebx >> 16) & 0xffff);

	printf("\n");
}

/* EAX = 0000 0015 */
static void handle_std_tsc(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t crystal_khz;

	if (!regs->eax || !regs->ebx)
		return;

	crystal_khz = regs->ecx / 1000;

	printf("Time Stamp Counter and Core Crystal Clock Information\n");

	if (!crystal_khz && state->curmax >= 0x16) {
		/* Skylake and Kaby Lake do not report the crystal Hz value, but we can
		 * infer it from the crystal ratio and CPU speed
		 */
		struct cpu_regs_t leaf16;
		ZERO_REGS(&leaf16);
		leaf16.eax = 0x16;
		state->cpuid_call(&leaf16, state);

		crystal_khz = leaf16.eax * 1000 * regs->eax / regs->ebx;
	}

	if (regs->ecx)
		printf("  Core crystal clock: %u Hz\n", regs->ecx);
	else
		printf("  Core crystal clock not enumerated\n");

	printf("  TSC to core crystal clock ratio: %u / %d\n", regs->ebx, regs->eax);

	if (crystal_khz)
		printf("  TSC frequency: %u kHz\n", crystal_khz * regs->ebx / regs->eax);

	printf("\n");
}

/* EAX = 0000 0016 */
static void handle_std_cpufreq(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if ((state->vendor & (VENDOR_INTEL)) == 0)
		return;

	if (!regs->eax && !regs->ebx && !regs->ecx)
		return;

	printf("Processor Frequency Information\n");
	if (regs->eax)
		printf("  Base frequency: %u MHz\n", regs->eax & 0xffff);
	if (regs->ebx)
		printf("  Maximum frequency: %u MHz\n", regs->ebx & 0xffff);
	if (regs->ecx)
		printf("  Bus (reference) frequency: %u MHz\n", regs->ecx & 0xffff);

	printf("\n");
}

/* Not fully implemented. Need to see some hardware that actually has this leaf
 * before I finish this.
 */

/* EAX = 0000 0018 */
static void handle_std_tlb(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t max_ecx = regs->eax;

	struct eax_tlb_t {
		unsigned reserved:32;
	};
	struct ebx_tlb_t {
		unsigned has_4k_pages:1;
		unsigned has_2m_pages:1;
		unsigned has_4m_pages:1;
		unsigned has_1g_pages:1;
		unsigned reserved:4;
		unsigned partitioning:3; /* 0: soft partitioning between logical processors sharing this structure */
		unsigned reserved_1:5;
		unsigned assoc:16;
	};
	struct ecx_tlb_t {
		unsigned sets:32;
	};
	struct edx_tlb_t {
		unsigned type:5;
		unsigned level:3;
		unsigned fully_assoc:1;
		unsigned reserved:5;
		unsigned max_threads_sharing:12; /* +1 encoded */
		unsigned reserved_1:6;
	};
	uint32_t i = 0;
	char buffer[256];

	if ((state->vendor & VENDOR_INTEL) == 0)
		return;

	if ((regs->edx & 0xf) == 0)
		return;

	printf("Deterministic Address Translation Parameters:\n");

	for (i = 0; i <= max_ecx; i++) {
		struct cache_desc_t tlb;
		//struct eax_tlb_t *eax = (struct eax_tlb_t *)&regs->eax;
		struct ebx_tlb_t *ebx = (struct ebx_tlb_t *)&regs->ebx;
		struct ecx_tlb_t *ecx = (struct ecx_tlb_t *)&regs->ecx;
		struct edx_tlb_t *edx = (struct edx_tlb_t *)&regs->edx;

		ZERO_REGS(regs);
		regs->eax = 0x18;
		regs->ecx = i;
		state->cpuid_call(regs, state);

		memset(&tlb, 0, sizeof(struct cache_desc_t));
		switch(edx->type) {
		case 0: continue; /* Invalid */
		case 1: tlb.type = DATA_TLB; break;
		case 2: tlb.type = CODE_TLB; break;
		case 3: tlb.type = SHARED_TLB; break;
		case 4: tlb.type = LOADONLY_TLB; break;
		case 5: tlb.type = STOREONLY_TLB; break;
		default: printf("  Unknown TLB type: %x (%d)\n\n", edx->type, edx->type); continue;
		}

		tlb.level = edx->level;
		if (ebx->has_4k_pages)
			tlb.attrs |=  PAGES_4K;
		if (ebx->has_2m_pages)
			tlb.attrs |=  PAGES_2M;
		if (ebx->has_4m_pages)
			tlb.attrs |=  PAGES_4M;
		if (ebx->has_1g_pages)
			tlb.attrs |=  PAGES_1G;
		tlb.assoc = edx->fully_assoc ? 0xFF : ebx->assoc;
		tlb.partitions = ebx->partitioning;
		tlb.size = ecx->sets;
		tlb.max_threads_sharing = edx->max_threads_sharing + 1;

		describe_cache(state->logical_in_socket, &tlb, buffer, sizeof(buffer), 2);
		printf("%s\n", buffer);
	}
}

/* EAX = 0000 001B */
static void handle_dump_std_1B(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i;
	struct eax_pconfig {
		unsigned leaftype:12;
		unsigned reserved:20;
	};

	/* Always print EAX=0x1b, ECX=0 response, even if we don't support PCONFIG. */
	state->cpuid_print(regs, state, TRUE);

	/* First check if PCONFIG is supported. */
	ZERO_REGS(regs);
	regs->eax = 0x07;
	state->cpuid_call(regs, state);
	if ((regs->edx & 0x00040000) == 0)
		return;

	for (i = 1; ; i++) {
		struct eax_pconfig *eax = (struct eax_pconfig *)&regs->eax;
		ZERO_REGS(regs);
		regs->eax = 0x1b;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		if (!eax->leaftype)
			break;
	}
}

/* EAX = 8000 0000 */
static void handle_ext_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->curmax = regs->eax;
	printf("Maximum extended CPUID leaf: 0x%08x\n\n", state->curmax);
}

/* EAX = 8000 0002 */
static void handle_ext_pname(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t base = (state->last_leaf.eax - 0x80000002) * 16;
	if (base == 0)
		memset(state->procname, 0, sizeof(state->procname));

	*(uint32_t *)&state->procname[base] = regs->eax;
	*(uint32_t *)&state->procname[base+4] = regs->ebx;
	*(uint32_t *)&state->procname[base+8] = regs->ecx;
	*(uint32_t *)&state->procname[base+12] = regs->edx;

	if (base == 32) {
		state->procname[47] = 0;
		squeeze(state->procname);
		printf("Processor Name: %s\n\n", state->procname);
	}
}

/* EAX = 8000 0005 */
static void handle_ext_amdl1cachefeat(struct cpu_regs_t *regs, __unused_variable struct cpuid_state_t *state)
{
	char desc_str[256];
	struct amd_l1_tlb_t {
		uint8_t itlb_ent;
		uint8_t itlb_assoc;
		uint8_t dtlb_ent;
		uint8_t dtlb_assoc;
	};
	struct amd_l1_cache_t {
		uint8_t linesize;
		uint8_t linespertag;
		uint8_t assoc;
		uint8_t size;
	};
	struct amd_l1_tlb_t *tlb;
	struct amd_l1_cache_t *cache;
	struct cache_desc_t desc;

	struct cpu_regs_t feat_check;
	int has_extended_topology;

	/* This is an AMD-only leaf. */
	if ((state->vendor & VENDOR_AMD) == 0)
		return;

	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	has_extended_topology = (feat_check.ecx & 0x400000) ? 1 : 0;

	if (regs->ebx || regs->eax)
		printf("L1 TLBs:\n");

	tlb = (struct amd_l1_tlb_t *)&regs->ebx;
	if (tlb->dtlb_ent) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = DATA_TLB;
		desc.assoc = tlb->dtlb_assoc;
		desc.size = tlb->dtlb_ent;
		desc.attrs = PAGES_4K;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}
	if (tlb->itlb_ent) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = CODE_TLB;
		desc.assoc = tlb->itlb_assoc;
		desc.size = tlb->itlb_ent;
		desc.attrs = PAGES_4K;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}

	tlb = (struct amd_l1_tlb_t *)&regs->eax;
	if (tlb->dtlb_ent) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = DATA_TLB;
		desc.assoc = tlb->dtlb_assoc;
		desc.size = tlb->dtlb_ent;
		desc.attrs = PAGES_2M | PAGES_4M;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}
	if (tlb->itlb_ent) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = CODE_TLB;
		desc.assoc = tlb->itlb_assoc;
		desc.size = tlb->itlb_ent;
		desc.attrs = PAGES_2M | PAGES_4M;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}

	if (has_extended_topology) {
		/* This leaf cannot contain all the relevant information because of
		 * the limited expressiveness of its design. Skip printing this in
		 * favor of extended topology when available.
		 */
		return;
	}

	if (regs->ecx || regs->edx)
		printf("L1 Caches:\n");

	cache = (struct amd_l1_cache_t *)&regs->ecx;
	if (cache->size) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = DATA;
		desc.assoc = cache->assoc;
		desc.size = cache->size;
		desc.linesize = cache->linesize;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}

	cache = (struct amd_l1_cache_t *)&regs->edx;
	if (cache->size) {
		memset(&desc, 0, sizeof(struct cache_desc_t));
		desc.level = 1;
		desc.type = CODE;
		desc.assoc = cache->assoc;
		desc.size = cache->size;
		desc.linesize = cache->linesize;
		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
	}

	printf("\n");
}

static const uint8_t amd_assoc_map[] = {
	/* 0x00 */ 0,
	/* 0x01 */ 1,
	/* 0x02 */ 2,
	/* 0x03 */ 0,
	/* 0x04 */ 4,
	/* 0x05 */ 0,
	/* 0x06 */ 8,
	/* 0x07 */ 0,
	/* 0x08 */ 16,
	/* 0x09 */ 0,
	/* 0x0A */ 32,
	/* 0x0B */ 48,
	/* 0x0C */ 64,
	/* 0x0D */ 96,
	/* 0x0E */ 128,
	/* 0x0F */ 0xff
};

/* EAX = 8000 0006 */
static void handle_ext_l2cachefeat(struct cpu_regs_t *regs, __unused_variable struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_INTEL) {
		/*
		 * Implemented below, but disabled because it's mostly
		 * worthless. Leaf 0x04 is far better at giving this information.
		 */
#if 0
		static const char *assoc[] = {
			/* 0x00 */ "Disabled",
			/* 0x01 */ "Direct mapped",
			/* 0x02 */ "2-way",
			/* 0x03 */ NULL,
			/* 0x04 */ "4-way",
			/* 0x05 */ NULL,
			/* 0x06 */ "8-way",
			/* 0x07 */ NULL,
			/* 0x08 */ "16-way",
			/* 0x09 */ NULL,
			/* 0x0A */ NULL,
			/* 0x0B */ NULL,
			/* 0x0C */ NULL,
			/* 0x0D */ NULL,
			/* 0x0E */ NULL,
			/* 0x0F */ "Fully associative"
		};

		struct l2cache_feat_t {
			unsigned linesize:8;
			unsigned reserved1:4;
			unsigned assoc:4;
			unsigned size:16;
		};

		struct l2cache_feat_t *feat = (struct l2cache_feat_t *)&regs->ecx;

		printf("L2 cache:\n"
		       "  %d%cB, %s associativity, %d byte line size\n\n",
		       feat->size > 1024 ? feat->size / 1024 : feat->size,
		       feat->size > 1024 ? 'M' : 'K',
		       assoc[feat->assoc] ? assoc[feat->assoc] : "Unknown",
		       feat->linesize);
#endif
	}

	if (state->vendor & VENDOR_AMD) {

		struct l2_tlb_t {
			unsigned itlb_size:12;
			unsigned itlb_assoc:4;
			unsigned dtlb_size:12;
			unsigned dtlb_assoc:4;
		};
		struct l2_cache_t {
			unsigned linesize:8;
			unsigned linespertag:4;
			unsigned assoc:4;
			unsigned size:16;
		};
		struct l3_cache_t {
			unsigned linesize:8;
			unsigned linespertag:4;
			unsigned assoc:4;
			unsigned reserved:2;
			unsigned size:14;
		};

		struct l2_tlb_t *tlb;
		struct l2_cache_t *l2_cache;
		struct l3_cache_t *l3_cache;

		struct cache_desc_t desc;
		char desc_str[256];

		struct cpu_regs_t feat_check;
		int has_extended_topology;

		ZERO_REGS(&feat_check);
		feat_check.eax = 0x80000001;
		state->cpuid_call(&feat_check, state);
		has_extended_topology = (feat_check.ecx & 0x400000) ? 1 : 0;

		printf("L2 TLBs:\n");

		tlb = (struct l2_tlb_t *)&regs->ebx;
		if (tlb->dtlb_size) {
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 2;
			desc.type = DATA_TLB;
			desc.assoc = tlb->dtlb_assoc == 0x6 ? 8 : 0;
			desc.size = tlb->dtlb_size;
			desc.attrs = PAGES_4K;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}
		if (tlb->itlb_size) {
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 2;
			desc.type = CODE_TLB;
			desc.assoc = tlb->itlb_assoc == 0x6 ? 8 : 0;
			desc.size = tlb->itlb_size;
			desc.attrs = PAGES_4K;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}

		tlb = (struct l2_tlb_t *)&regs->eax;
		if (tlb->dtlb_size) {
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 2;
			desc.type = DATA_TLB;
			desc.assoc = tlb->dtlb_assoc == 0x4 ? 4 : 0;
			desc.size = tlb->dtlb_size;
			desc.attrs = PAGES_2M | PAGES_4M;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}
		if (tlb->itlb_size) {
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 2;
			desc.type = CODE_TLB;
			desc.assoc = tlb->itlb_assoc == 0x6 ? 8 : 0;
			desc.size = tlb->itlb_size;
			desc.attrs = PAGES_2M | PAGES_4M;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}

		if (has_extended_topology) {
			/* This leaf cannot contain all the relevant information because of
			 * the limited expressiveness of its design. Skip printing this in
			 * favor of extended topology when available.
			 */
			return;
		}

		printf("L2 and L3 caches:\n");

		l2_cache = (struct l2_cache_t *)&regs->ecx;
		if (l2_cache->size) {
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 2;
			desc.type = UNIFIED;
			desc.size = l2_cache->size;
			desc.assoc = l2_cache->assoc < NELEM(amd_assoc_map) ? amd_assoc_map[l2_cache->assoc] : 0;
			desc.linesize = l2_cache->linesize;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}

		l3_cache = (struct l3_cache_t *)&regs->edx;
		if (l3_cache->size) {
			uint32_t size = l3_cache->size * 512;
			if ( l3_cache->size == 0x0003 ||
			    (l3_cache->size >= 0x0005 && l3_cache->size <= 0x0007) ||
			    (l3_cache->size >= 0x0009 && l3_cache->size <= 0x000F) ||
			    (l3_cache->size >= 0x0011 && l3_cache->size <= 0x001F))
			{
				size /= 2;
			}
			memset(&desc, 0, sizeof(struct cache_desc_t));
			desc.level = 3;
			desc.type = UNIFIED;
			desc.size = size;
			desc.assoc = l3_cache->assoc < NELEM(amd_assoc_map) ? amd_assoc_map[l3_cache->assoc] : 0;
			desc.linesize = l3_cache->linesize;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}
		printf("\n");
	}
}

/* EAX = 8000 0008 */
static void handle_ext_0008(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	/* Long mode address size information */
	struct eax_addrsize {
		unsigned physical:8;
		unsigned linear:8;
		unsigned guestphysical:8;
		unsigned reserved:8;
	};

	struct eax_addrsize *eax = (struct eax_addrsize *)&regs->eax;

	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;

	if (eax->guestphysical)
		printf("Guest physical address size: %d bits\n", eax->guestphysical);
	printf("Physical address size: %d bits\n", eax->physical);
	printf("Linear address size: %d bits\n", eax->linear);
	printf("\n");

	if ((state->vendor & VENDOR_AMD) != 0) {
		struct ecx_apiccore {
			unsigned nc:8;
			unsigned reserved_1:4;
			unsigned apicidcoreidsize:4;
			unsigned perftscsize:2;
			unsigned reserved_2:14;
		};

		struct ecx_apiccore *ecx = (struct ecx_apiccore *)&regs->ecx;

		uint32_t nc = ecx->nc + 1;
		uint32_t tscsize = ecx->perftscsize;
		uint32_t mnc = (ecx->apicidcoreidsize > 0) ? (1u << ecx->apicidcoreidsize) : nc;

		switch(tscsize) {
		case 0: tscsize = 40; break;
		case 1: tscsize = 48; break;
		case 2: tscsize = 56; break;
		case 3: tscsize = 64; break;
		}

		state->logical_in_socket = nc;
		printf("Core count: %u\n", nc);
		printf("Performance time-stamp counter size: %u bits\n", tscsize);
		printf("Maximum core count: %u\n", mnc);
		print_features(regs, state);
		printf("\n");
	}
}

/* EAX = 8000 000A */
static void handle_ext_svm(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_svm {
		unsigned svmrev:8;
		unsigned reserved:24;
	};

	struct ebx_svm {
		unsigned nasid:32;
	};

	struct eax_svm *eax = (struct eax_svm *)&regs->eax;
	struct ebx_svm *ebx = (struct ebx_svm *)&regs->ebx;
	struct cpu_regs_t feat_check;

	if (!(state->vendor & VENDOR_AMD))
		return;

	/* First check for SVM feature bit. */
	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	if (!(feat_check.ecx & 0x04))
		return;

	/* This got clobbered by the feature check. */
	state->last_leaf.eax = 0x8000000A;
	state->last_leaf.ecx = 0;

	printf("SVM Features and Revision Information:\n");
	printf("  Revision: %u\n", eax->svmrev);
	printf("  NASID: %u\n", ebx->nasid);
	printf("  Features:\n");
	print_features(regs, state);
	printf("\n");
}

/* EAX = 8000 0005 */
static void handle_ext_amd_1g_tlb(struct cpu_regs_t *regs, __unused_variable struct cpuid_state_t *state)
{
	char desc_str[256];
	struct amd_1g_tlb_t {
		unsigned itlb_ent:12;
		unsigned itlb_assoc:4;
		unsigned dtlb_ent:12;
		unsigned dtlb_assoc:4;
	};
	struct amd_1g_tlb_t *tlb;
	struct cache_desc_t desc;
	int i;

	/* This is an AMD-only leaf. */
	if ((state->vendor & VENDOR_AMD) == 0)
		return;

	memset(&desc, 0, sizeof(struct cache_desc_t));
	desc.attrs = PAGES_1G;

	if (regs->eax || regs->ebx)
		printf("1GB page TLBs:\n");

	for (i = 0; i < 2; i++) {
		tlb = (struct amd_1g_tlb_t *)&regs->regs[i];
		if (tlb->dtlb_ent) {
			desc.level = i + 1;
			desc.type = DATA_TLB;
			desc.assoc = tlb->dtlb_assoc < NELEM(amd_assoc_map) ? amd_assoc_map[tlb->dtlb_assoc] : 0;
			desc.size = tlb->dtlb_ent;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}
		if (tlb->itlb_ent) {
			desc.level = i + 1;
			desc.type = CODE_TLB;
			desc.assoc = tlb->itlb_assoc < NELEM(amd_assoc_map) ? amd_assoc_map[tlb->itlb_assoc] : 0;
			desc.size = tlb->itlb_ent;
			printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));
		}
	}
}

/* EAX = 8000 001A */
static void handle_ext_perf_opt_feat(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_AMD))
		return;
	printf("Performance Optimization identifiers:\n");
	print_features(regs, state);
	printf("\n");
}

/* EAX = 8000 001B */
static void handle_ext_ibs_feat(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_AMD))
		return;
	if (!regs->eax)
		return;
	printf("Instruction Based Sampling identifiers:\n");
	print_features(regs, state);
	printf("\n");
}

/* EAX = 8000 001D */
static void handle_dump_ext_1D(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct cpu_regs_t feat_check;
	uint32_t i = 0, has_extended_topology = 0;

	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	has_extended_topology = (feat_check.ecx & 0x400000) ? 1 : 0;

	if (!has_extended_topology)
		state->cpuid_print(regs, state, TRUE);
	else
		while (1) {
			ZERO_REGS(regs);
			regs->eax = 0x8000001D;
			regs->ecx = i;
			state->cpuid_call(regs, state);
			if (regs->eax == 0)
				break;
			state->cpuid_print(regs, state, TRUE);
			i++;
		}
}

/* EAX = 8000 001D */
static void handle_ext_cacheprop(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_cache {
		unsigned type:5;
		unsigned level:3;
		unsigned selfinit:1;
		unsigned fullyassoc:1;
		unsigned reserved_1:4;
		unsigned sharing:12;
		unsigned reserved_2:6;
	};
	struct ebx_cache {
		unsigned linesize:12;
		unsigned partitions:10;
		unsigned ways:10;
	};
	struct ecx_cache {
		unsigned sets:32;
	};

	struct eax_cache *eax = (struct eax_cache *)&regs->eax;
	struct ebx_cache *ebx = (struct ebx_cache *)&regs->ebx;
	struct ecx_cache *ecx = (struct ecx_cache *)&regs->ecx;
	struct cpu_regs_t feat_check;
	unsigned int i = 1;

	if (!(state->vendor & VENDOR_AMD))
		return;

	/* First check for Extended Topology feature bit. */
	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	if (!(feat_check.ecx & 0x400000))
		return;

	printf("AMD Extended Cache Topology:\n");
	while (1) {
		struct cache_desc_t desc;
		char desc_str[512];
		uint32_t size;

		if (eax->type == 0)
			break;

		size = (ebx->partitions + 1) * (ebx->linesize + 1) * (ebx->ways + 1) * (ecx->sets + 1);
		size /= 1024;

		desc.level = eax->level + L0;
		desc.type = eax->type + DATA - 1;
		desc.size = size;
		desc.attrs = (eax->selfinit ? SELF_INIT : 0) |
		             ((regs->edx & 0x01) ? WBINVD_NOT_INCLUSIVE : 0) |
		             ((regs->edx & 0x02) ? INCLUSIVE : 0);
		desc.assoc = eax->fullyassoc ? 0xff : ebx->ways + 1;
		desc.linesize = ebx->linesize + 1;
		desc.partitions = ebx->partitions + 1;
		desc.max_threads_sharing = eax->sharing + 1;

		printf("%s\n", describe_cache(state->logical_in_socket, &desc, desc_str, sizeof(desc_str), 2));

		ZERO_REGS(regs);
		regs->eax = 0x8000001D;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		i++;
		if (!regs->eax)
			break;
	}
}

/* EAX = 8000 001E */
static void handle_ext_extapic(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_extapic {
		unsigned extapicid:32;
	};
	struct ebx_computeunit {
		unsigned compute_unit_id:8;
		unsigned cores_per_unit:2;
		unsigned reserved:22;
	};
	struct ecx_nodeid {
		unsigned nodeid:8;
		unsigned nodes_per_processor:3;
		unsigned reserved:21;
	};

	struct eax_extapic *eax = (struct eax_extapic *)&regs->eax;
	struct ebx_computeunit *ebx = (struct ebx_computeunit *)&regs->ebx;
	struct ecx_nodeid *ecx = (struct ecx_nodeid *)&regs->ecx;
	struct cpu_regs_t feat_check;

	if (!(state->vendor & VENDOR_AMD))
		return;

	/* First check for Extended Topology feature bit. */
	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	if (!(feat_check.ecx & 0x400000))
		return;

	printf("AMD Extended Topology:\n");
	printf("  Extended APIC ID: 0x%08x\n", eax->extapicid);
	printf("  Compute unit ID: %d\n", ebx->compute_unit_id + 1);
	printf("  Cores per unit: %d\n", ebx->cores_per_unit);
	printf("  Node ID: %d\n", ecx->nodeid);

	/* Only defined for 0b0 and 0b1 right now. */
	if (ecx->nodes_per_processor < 2) {
		printf("  Nodes per processor: %d\n", ecx->nodes_per_processor + 1);
	} else {
		printf("  Nodes per processor: UNKNOWN (0x%02x)\n", ecx->nodes_per_processor);
	}
}

/* EAX = 8000 0020 */
static void handle_dump_ext_20(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->cpuid_print(regs, state, TRUE);

	ZERO_REGS(regs);
	regs->eax = 0x80000020;
	regs->ecx = 1;
	state->cpuid_call(regs, state);
	state->cpuid_print(regs, state, TRUE);
}

/* EAX = 8086 0000 */
static void handle_tmta_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13] ALIGNED(4);
	size_t i;

	state->curmax = regs->eax;

	if (!(state->vendor & VENDOR_TRANSMETA))
		return;

	printf("Maximum Transmeta CPUID leaf: 0x%08x\n\n", state->curmax);

	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->edx;
	*(uint32_t *)(&buf[8]) = regs->ecx;
	buf[12] = 0;

	for (i = 0; i < sizeof(buf); i++) {
		/* End of vendor string */
		if (buf[i] == 0)
			break;

		/* Character outside printable range */
		if (buf[i] < 0x20 || buf[i] > 0x7E)
			buf[i] = '.';
	}

	buf[12] = 0;

	printf("CPU vendor string: '%s'\n\n", buf);
}

/* EAX = 8086 0003 through EAX = 8086 0006 */
static void handle_tmta_cmsinfo(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t base = (state->last_leaf.eax - 0x80860003) * 16;

	if (!(state->vendor & VENDOR_TRANSMETA))
		return;

	if (base == 0)
		memset(state->cmsinfo, 0, sizeof(state->cmsinfo));

	*(uint32_t *)&state->cmsinfo[base] = regs->eax;
	*(uint32_t *)&state->cmsinfo[base+4] = regs->ebx;
	*(uint32_t *)&state->cmsinfo[base+8] = regs->ecx;
	*(uint32_t *)&state->cmsinfo[base+12] = regs->edx;

	if (base == 48) {
		state->cmsinfo[63] = 0;
		squeeze(state->cmsinfo);
		printf("CMS Information: %s\n\n", state->cmsinfo);
	}
}

/* EAX = c000 0000 */
static void handle_centaur_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->curmax = regs->eax;

	if (!(state->vendor & VENDOR_CENTAUR))
		return;

	printf("Maximum Centaur CPUID leaf: 0x%08x\n\n", state->curmax);
}

/* EAX = 4000 0000 */
static void handle_vmm_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13];
	size_t i;

	state->curmax = regs->eax;

	if (state->curmax < 0x40000000)
		return;
	if (state->curmax > 0x4000FFFF)
		return;

	printf("Maximum hypervisor CPUID leaf: 0x%08x\n\n", state->curmax);

	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->ecx;
	*(uint32_t *)(&buf[8]) = regs->edx;

	for (i = 0; i < sizeof(buf); i++) {
		/* End of vendor string */
		if (buf[i] == 0)
			break;

		/* Character outside printable range */
		if (buf[i] < 0x20 || buf[i] > 0x7E)
			buf[i] = '.';
	}

	buf[12] = 0;

	printf("Hypervisor vendor string: '%s'\n\n", buf);

	if (strcmp(buf, "XenVMMXenVMM") == 0) {
		state->vendor |= VENDOR_HV_XEN;
		printf("Xen hypervisor detected\n\n");
	} else if (strcmp(buf, "VMwareVMware") == 0) {
		state->vendor |= VENDOR_HV_VMWARE;
		printf("VMware hypervisor detected\n\n");
	} else if (strcmp(buf, "KVMKVMKVM") == 0) {
		state->vendor |= VENDOR_HV_KVM;
		printf("KVM hypervisor detected\n\n");
	} else if (strcmp(buf, "Microsoft Hv") == 0) {
		state->vendor |= VENDOR_HV_HYPERV;
		printf("Hyper-V detected\n\n");
	} else if (strcmp(buf, " lrpepyh  vr") == 0) {
		state->vendor |= VENDOR_HV_PARALLELS;
		printf("Parallels Desktop detected\n\n");
	} else if (strcmp(buf, "bhyve bhyve ") == 0) {
		state->vendor |= VENDOR_HV_BHYVE;
		printf("BHYVE hypervisor detected\n\n");
	}
}

/* EAX = 4000 0001 */
static void handle_vmm_leaf01(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_HV_XEN) {
		printf("Xen version: %d.%d\n\n", regs->eax >> 16, regs->eax & 0xFFFF);
	}
	if (state->vendor & VENDOR_HV_KVM) {
		print_features(regs, state);
		printf("\n");
	}
	if (state->vendor & VENDOR_HV_HYPERV) {
		char buf[5];
		buf[4] = 0;
		*(uint32_t *)(&buf[0]) = regs->eax;
		printf("Hypervisor interface identification: '%s'\n\n", buf);
	} else if (state->vendor & VENDOR_HV_GENERIC
			&& regs->eax == 0x31237648 /* "Hv#1" */) {
		state->vendor |= VENDOR_HV_HYPERV;
		printf("Hyper-V compliant hypervisor detected\n\n");
	}
}

/* EAX = 4000 0002 */
static void handle_vmm_leaf02(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_HV_XEN) {
		printf("Xen features:\n"
			   "  Hypercall transfer pages: %d\n"
			   "  MSR base address: 0x%08x\n\n",
			   regs->eax,
			   regs->ebx);
	}
	if (state->vendor & VENDOR_HV_HYPERV) {
		struct ebx_version {
			unsigned minor:16;
			unsigned major:16;
		};
		//struct edx_service {
		//	unsigned service_number:24;
		//	unsigned service_branch:8;
		//};

		struct ebx_version *ebx = (struct ebx_version *)(&regs->ebx);
		//struct edx_service *edx = (struct edx_service *)(&regs->edx);

		printf("Version: %d.%d (build %d)", ebx->major, ebx->minor, regs->eax);
		if (regs->ecx)
			printf(" Service Pack %d", regs->ecx);

		printf("\n\n");
	}
}

/* EAX = 4000 0003 */
static void handle_dump_vmm_leaf03(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13];
	struct cpu_regs_t hv_vendor_regs;
	uint32_t i, eax, max_ecx;

	/* First, detect whether this is Xen or not, as that will affect whether we
	 * use ecx indexing for this leaf.
	 */
	ZERO_REGS(&hv_vendor_regs);
	hv_vendor_regs.eax = 0x40000000;
	state->cpuid_call(&hv_vendor_regs, state);

	*(uint32_t *)(&buf[0]) = hv_vendor_regs.ebx;
	*(uint32_t *)(&buf[4]) = hv_vendor_regs.ecx;
	*(uint32_t *)(&buf[8]) = hv_vendor_regs.edx;

	for (i = 0; i < sizeof(buf); i++) {
		/* End of vendor string */
		if (buf[i] == 0)
			break;

		/* Character outside printable range */
		if (buf[i] < 0x20 || buf[i] > 0x7E)
			buf[i] = '.';
	}

	buf[12] = 0;

	if (strcmp(buf, "XenVMMXenVMM") == 0)
		state->vendor |= VENDOR_HV_XEN;

	/*
	 * If it's Xen, we use a fixed max subleaf index of 2.
	 */
	eax = 0x40000003;
	max_ecx = (state->vendor & VENDOR_HV_XEN) ? 2 : 0;
	i = 0;
	while (i <= max_ecx) {
		ZERO_REGS(regs);
		regs->eax = eax;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

static const char *xen_tsc_mode_name(uint32_t _val)
{
	switch (_val) {
	case 0:
		return "emulate if necessary";
	case 1:
		return "emulate";
	case 2:
		return "no emulation";
	case 3:
		return "no emulation + TSC_AUX support";
	default:
		return "unknown";
	}
}

/* EAX = 4000 0003 */
static void handle_vmm_leaf03(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_HV_XEN) {
		uint32_t eax = state->last_leaf.eax;
		uint32_t max_ecx = (state->vendor & VENDOR_HV_XEN) ? 2 : 0;
		uint32_t i = 0;
		printf("Xen TSC configuration:\n");
		while (i <= max_ecx) {
			ZERO_REGS(regs);
			regs->eax = eax;
			regs->ecx = i;
			state->cpuid_call(regs, state);

			switch(i) {
			case 0:
				printf("  TSC mode: %d (%s)\n", regs->ebx, xen_tsc_mode_name(regs->ebx));
				if (regs->ecx)
					printf("  Guest TSC frequency: %dMHz\n", regs->ecx / 1000);
				printf("  Guest TSC incarnation: %d\n", regs->edx);
				break;
			case 1:
				printf("  TSC offset: 0x%08x%08x\n", regs->ebx, regs->eax);
				if (regs->ecx)
					printf("  TSC multiplier for ns conversion: 0x%08x\n", regs->ecx);
				if (regs->edx)
					printf("  TSC shift for ns conversion: 0x%08x\n", regs->edx);
				break;
			case 2:
				if (regs->eax)
					printf("  Host TSC frequency: %dMHz\n", regs->eax / 1000);
				break;
			}

			i++;
		}
		printf("\n");
	} else if (state->vendor & VENDOR_HV_HYPERV) {
		print_features(regs, state);
		printf("\n");
	}
}

/* EAX = 4000 0004 */
static void handle_vmm_leaf04(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_HV_XEN) {
		print_features(regs, state);
		if (regs->eax & ((1u << 3) | (1u << 4))) {
			printf("Xen HVM domain info:\n");
			if (regs->eax & (1u << 3))
				printf("  VCPU ID: %d\n", regs->ebx);
			if (regs->eax & (1u << 4))
				printf("  Domain ID: %d\n", regs->ecx);
		}
		printf("\n");
	} else if (state->vendor & VENDOR_HV_HYPERV) {
		struct ecx_addressing {
			unsigned physbits:7;
			unsigned reserved:25;
		};

		struct ecx_addressing *ecx = (struct ecx_addressing *)(&regs->ecx);
		if (ecx->physbits)
			printf("Physical address bits in hardware: %d\n", ecx->physbits);

		printf("Spinlock attempts before notifying hypervisor: ");
		if (regs->ebx == 0xFFFFFFFF)
			printf("never notify\n\n");
		else
			printf("%d\n\n", regs->ebx);

		if (regs->eax) {
			print_features(regs, state);
			printf("\n");
		}
	}
}

/* EAX = 4000 0005 */
static void handle_vmm_leaf05(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_HYPERV))
		return;
	if (!(regs->eax || regs->ebx || regs->ecx))
		return;
	if (regs->eax)
		printf("Maximum virtual processors: %d\n", regs->eax);
	if (regs->ebx)
		printf("Maximum logical processors: %d\n", regs->ebx);
	if (regs->ecx)
		printf("Maximum interrupt vectors for intremap: %d\n", regs->ecx);
	printf("\n");
}

/* EAX = 4000 0006 */
static void handle_vmm_leaf06(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_HYPERV))
		return;
	if (print_features(regs, state))
		printf("\n");
}

/* EAX = 4000 0007 */
static void handle_hyperv_leaf07(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_HYPERV))
		return;
	if (!(regs->eax || regs->ebx || regs->ecx))
		return;
	printf("Hyper-V enlightenments available to the root partition only:\n");
	print_features(regs, state);
	printf("\n");
}

/* EAX = 4000 0008 */
static void handle_hyperv_leaf08(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_HYPERV))
		return;
	if (!regs->eax)
		return;
	if (print_features(regs, state))
		printf("\n");
	printf("Maximum PASID space PASID count: %d\n\n", regs->eax >> 12);
}

/* EAX = 4000 0009 */
static void handle_hyperv_leaf09(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_HYPERV))
		return;
	if (!(regs->eax || regs->edx))
		return;
	printf("Hyper-V nested feature identification:\n");
	print_features(regs, state);
	printf("\n");
}

/* EAX = 4000 000A */
static void handle_hyperv_leaf0A(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_HV_HYPERV) {
		struct eax_version {
			unsigned low:8;
			unsigned high:8;
		};

		struct eax_version *eax = (struct eax_version *)(&regs->eax);

		printf("Enlightened VMCS version low : %d\n", eax->low);
		printf("Enlightened VMCS version high: %d\n", eax->high);
		print_features(regs, state);
		printf("\n");
	}
}

/* EAX = 4000 0010 */
static void handle_vmware_leaf10(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (!(state->vendor & VENDOR_HV_VMWARE))
		return;
	printf("TSC frequency: %4.2fMHz\n"
	       "Bus (local APIC timer) frequency: %4.2fMHz\n\n",
	       (float)regs->eax / 1000.0f,
		   (float)regs->ebx / 1000.0f);
}

/* vim: set ts=4 sts=4 sw=4 noet: */
