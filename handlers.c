
#include "prefix.h"

#include "cache.h"
#include "feature.h"
#include "handlers.h"
#include "state.h"
#include "threads.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void handle_features(struct cpu_regs_t *regs, struct cpuid_state_t *state);

void handle_std_base(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_cache02(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_psn(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_cache04(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_monitor(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_power(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_extfeat(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_std_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state);

void handle_ext_base(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_pname(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_amdl1cachefeat(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_l2cachefeat(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_0007(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_0008(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_ext_svm(struct cpu_regs_t *regs, struct cpuid_state_t *state);

void handle_vmm_base(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_xen_version(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_xen_leaf02(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_xen_leaf03(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_vmware_leaf10(struct cpu_regs_t *regs, struct cpuid_state_t *state);

void handle_dump_base(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_dump_std_04(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_dump_std_07(struct cpu_regs_t *regs, struct cpuid_state_t *state);
void handle_dump_std_0B(struct cpu_regs_t *regs, struct cpuid_state_t *state);

const struct cpuid_leaf_handler_index_t dump_handlers[] =
{
	/* Standard levels */
	{0x00000000, handle_dump_base},
	{0x00000004, handle_dump_std_04},
	{0x00000007, handle_dump_std_07},
	{0x0000000B, handle_dump_std_0B},

	/* Hypervisor levels */
	{0x40000000, handle_dump_base},

	/* Extended levels */
	{0x80000000, handle_dump_base},

	{0, 0}
};

const struct cpuid_leaf_handler_index_t decode_handlers[] =
{
	/* Standard levels */
	{0x00000000, handle_std_base},
	{0x00000001, handle_features},
	{0x00000002, handle_std_cache02},
	{0x00000003, handle_std_psn},
	{0x00000004, handle_std_cache04},
	{0x00000005, handle_std_monitor},
	{0x00000006, handle_std_power},
	{0x00000007, handle_std_extfeat},
	{0x0000000B, handle_std_x2apic},

	/* Hypervisor levels */
	{0x40000000, handle_vmm_base},
	{0x40000001, handle_xen_version},
	{0x40000002, handle_xen_leaf02},
	{0x40000003, handle_xen_leaf03},
	{0x40000003, handle_vmware_leaf10},

	/* Extended levels */
	{0x80000000, handle_ext_base},
	{0x80000001, handle_features},
	{0x80000002, handle_ext_pname},
	{0x80000003, handle_ext_pname},
	{0x80000004, handle_ext_pname},
	{0x80000005, handle_ext_amdl1cachefeat},
	{0x80000006, handle_ext_l2cachefeat},
	{0x80000007, handle_ext_0007},
	{0x80000008, handle_ext_0008},
	{0x8000000A, handle_ext_svm},

	{0, 0}
};

/* EAX = 0000 0000 | EAX = 4000 0000 | EAX = 8000 0000 */
void handle_dump_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->curmax = regs->eax;
	state->cpuid_print(regs, state, FALSE);
}

/* EAX = 0000 0000 */
void handle_std_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13];
	state->curmax = regs->eax;
	printf("Maximum basic CPUID leaf: 0x%08x\n\n", state->curmax);
	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->edx;
	*(uint32_t *)(&buf[8]) = regs->ecx;
	buf[12] = 0;

	/*
	 * Ideally, we want to do this sometime in the future, but
	 * our printout format is currently designed to be handling
	 * only one vendor.
	 */
#if 0
	if (ignore_vendor) {
		state->vendor = VENDOR_ANY;
		return;
	}
#endif

	if (strcmp(buf, "GenuineIntel") == 0)
		state->vendor = VENDOR_INTEL;
	else if (strcmp(buf, "AuthenticAMD") == 0)
		state->vendor = VENDOR_AMD;
	else if (strcmp(buf, "GenuineTMx86") == 0)
		state->vendor = VENDOR_TRANSMETA;
	else if (strcmp(buf, "CyrixInstead") == 0)
		state->vendor = VENDOR_CYRIX;
	else
		state->vendor = VENDOR_UNKNOWN;
}

/* EAX = 8000 0001 | EAX = 0000 0001 */
void handle_features(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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
		*(uint32_t *)(&state->sig) = regs->eax;

		/* Model is calculated differently on Intel/AMD. */
		model = state->sig.model;
		if (state->vendor & VENDOR_INTEL) {
			model += ((state->sig.family == 0xf || state->sig.family == 0x6) ? state->sig.extmodel << 4 : 0);
		} else if (state->vendor & VENDOR_AMD) {
			model += (model == 0xf ? state->sig.extmodel << 4 : 0);
		}

		printf("Signature:  0x%08x\n"
		       "  Family:   0x%02x (%d)\n"
		       "  Model:    0x%02x (%d)\n"
		       "  Stepping: 0x%02x (%d)\n\n",
		       *(uint32_t *)&state->sig,
		       state->sig.family + state->sig.extfamily,
		       state->sig.family + state->sig.extfamily,
		       model,
		       model,
		       state->sig.stepping,
		       state->sig.stepping);
		printf("Local APIC: %d\n"
		       "Logical processor count: %d\n"
		       "CLFLUSH size: %d\n"
		       "Brand ID: %d\n\n",
		       ebx->localapicid,
		       ebx->logicalcount,
		       ebx->clflushsz << 3,
		       ebx->brandid);
	}
	print_features(regs, state);
	printf("\n");
}

/* EAX = 0000 0002 */
void handle_std_cache02(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint8_t i, m = regs->eax & 0xFF;
	struct cpu_regs_t *rvec = NULL;
	uint8_t *cdesc;
	if ((state->vendor & (VENDOR_INTEL | VENDOR_CYRIX)) == 0)
		return;

	/* I don't think this ever happens, but just in case... */
	if (m < 1)
		return;

	rvec = malloc(sizeof(struct cpu_regs_t) * m);
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

	/* Scan for 0xFF descriptor, which says to ignore leaf 0x02 */
	cdesc = (uint8_t *)rvec;
	while (cdesc <= (uint8_t *)rvec + (sizeof(struct cpu_regs_t) * m))
		if (*cdesc++ == 0xFF)
			goto err;

	/* Printout time. */
	printf("Cache descriptors:\n");
	regs = rvec;
	for (i = 0; i < m; i++) {
		print_intel_caches(regs, &state->sig);
		regs++;
	}
	printf("\n");

err:
	free(rvec);
}

/* EAX = 0000 0003 */
void handle_std_psn(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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
static const char *cache04_type(uint8_t type)
{
	const char *types[] = {
		"null",
		"data",
		"code",
		"unified"
	};
	if (type > 3)
		return "unknown";
	return types[type];
}

/* EAX = 0000 0004 */
void handle_std_cache04(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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
		struct eax_cache04_t *eax;
		struct ebx_cache04_t *ebx;
		uint32_t cacheSize;
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		state->cpuid_call(regs, state);

		/* This is a non-official check. With other leafs (i.e. 0x0B),
		   some extra information comes through, past the termination
		   condition. I want to show all the information the CPU provides,
		   even if it's not specified by the Intel docs. */
		if (!regs->eax && !regs->ebx && !regs->ecx && !regs->edx)
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

		printf("  %3u%cB L%d %s cache\n",
		       cacheSize > 1024 ? cacheSize / 1024 : cacheSize,
		       cacheSize > 1024 ? 'M' : 'K',
		       eax->level,
		       cache04_type(eax->type));

		if (eax->fully_associative) {
			printf("        fully associative\n");
		} else {
			printf("        %d-way set associative\n",
			       ebx->assoc + 1);
		}

		printf("        %d byte line size\n"
		       "        %d partitions\n"
		       "        %d sets\n"
		       "        shared by max %d threads\n\n",
		       ebx->line_size + 1,
		       ebx->partitions + 1,
		       regs->ecx + 1,
		       eax->max_threads_sharing + 1);

		/* This is the official termination condition for this leaf. */
		if (!(regs->eax & 0xF))
			break;

		i++;
	}
}

/* EAX = 0000 0004 */
void handle_dump_std_04(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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
void handle_std_monitor(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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
		unsigned c:20;
		unsigned reserved:12;
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
		for (i = 0; i < 5; i++) {
			printf("  C%d sub C-states supported by MWAIT: %d\n", i, (edx->c & (0xF << (i * 4))) >> (i * 4));
		}
	}
no_enumeration:
	printf("\n");
}

/* EAX = 0000 0006 */
void handle_std_power(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_power_t {
		unsigned temp_sensor:1;
		unsigned turbo_boost:1;
		unsigned arat:1;
		unsigned reserved_1:1;
		unsigned pln:1;
		unsigned ecmd:1;
		unsigned ptm:1;
		unsigned reserved_2:25;
	};
	struct ebx_power_t {
		unsigned dts_thresholds:4;
		unsigned reserved:28;
	};
	struct ecx_power_t {
		unsigned hcf:1;
		unsigned reserved_1:2;
		unsigned perf_bias:1;
		unsigned reserved_2:28;
	};
	struct eax_power_t *eax = (struct eax_power_t *)&regs->eax;
	struct ebx_power_t *ebx = (struct ebx_power_t *)&regs->ebx;
	struct ecx_power_t *ecx = (struct ecx_power_t *)&regs->ecx;

	if ((state->vendor & VENDOR_INTEL) == 0)
		return;

	/* If we don't have anything to print, skip. */
	if (!(regs->eax || regs->ebx || regs->ecx))
		return;

	printf("Intel Thermal and Power Management Features:\n");
	if (eax->temp_sensor)
		printf("  Digital temperature sensor\n");
	if (eax->turbo_boost)
		printf("  Intel Turbo Boost Technology\n");
	if (eax->arat)
		printf("  APIC timer always running\n");
	if (eax->pln)
		printf("  Power limit notification controls\n");
	if (eax->ecmd)
		printf("  Clock modulation duty cycle extension\n");
	if (eax->ptm)
		printf("  Package thermal management\n");
	if (ebx->dts_thresholds)
		printf("  Interrupt thresholds in DTS: %d\n", ebx->dts_thresholds);
	if (ecx->hcf)
		printf("  Hardware Coordination Feedback Capability\n");
	if (ecx->perf_bias)
		printf("  Performance-energy bias preference\n");
	printf("\n");
}

/* EAX = 0000 0007 */
void handle_std_extfeat(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0, m = regs->eax;
	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;
	if (!(regs->eax || regs->ebx || regs->ecx || regs->edx))
		return;
	printf("Structured Extended Feature Flags:\n");
	while (i <= m) {
		ZERO_REGS(regs);
		regs->eax = 0x7;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		print_features(regs, state);
		i++;
	}
	printf("\n");
}

/* EAX = 0000 0007 */
void handle_dump_std_07(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0, m = regs->eax;
	while (i <= m) {
		ZERO_REGS(regs);
		regs->eax = 0x7;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		i++;
	}
}

/* EAX = 0000 000B */
static const char *x2apic_level_type(uint8_t type)
{
	const char *types[] = {
		"Invalid",
		"Thread",
		"Core",
		"Unknown"
	};
	if (type > 2) type = 3;
	return types[type];
}

/* EAX = 0000 000B */
void handle_std_x2apic(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i;
	uint32_t threads, cores;

	if ((state->vendor & VENDOR_INTEL) == 0)
		return;
	if (!regs->eax && !regs->ebx)
		return;
	printf("Processor Topology:\n");

	/* Inferrence */
	i = 0;
	threads = cores = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (!(regs->eax || regs->ebx || regs->ecx || regs->edx))
			break;
		switch ((regs->ecx >> 8) & 0xff) {
		case 1: /* Thread level */
			threads = regs->ebx & 0xffff;
			break;
		case 2: /* Core level */
			cores = regs->ebx & 0xffff;
			break;
		}
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
	if (threads && cores) {
		uint32_t total = state->thread_count(state);
		cores /= threads;
		printf("  Inferred information:\n");
		printf("    Logical total:       %u%s\n", total, (total >= cores * threads) ? "" : " (?)");
		printf("    Logical per socket:  %u\n", cores * threads);
		printf("    Cores per socket:    %u\n", cores);
		printf("    Threads per core:    %u\n\n", threads);
	}

	/* Intel specification */
	i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		if (!(regs->eax || regs->ebx || regs->ecx || regs->edx))
			break;
		printf("  Bits to shift: %d\n"
		       "  Logical at this level: %d\n"
		       "  Level number: %d\n"
		       "  Level type: %d (%s)\n"
		       "  x2APIC ID: %d\n\n",
		       regs->eax & 0x1f,
		       regs->ebx & 0xffff,
		       regs->ecx & 0xff,
		       (regs->ecx >> 8) & 0xff,
		       x2apic_level_type((regs->ecx >> 8) & 0xff),
		       regs->edx);
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
}

/* EAX = 0000 000B */
void handle_dump_std_0B(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		state->cpuid_call(regs, state);
		state->cpuid_print(regs, state, TRUE);
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
}

/* EAX = 8000 0000 */
void handle_ext_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	state->curmax = regs->eax;
	printf("Maximum extended CPUID leaf: 0x%08x\n\n", state->curmax);
}

/* EAX = 8000 0002 */
void handle_ext_pname(struct cpu_regs_t *regs, struct cpuid_state_t *state)
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

static const char *amd_associativity(char *buffer, uint8_t assoc)
{
	switch (assoc) {
	case 0x00:
		return "Reserved";
	case 0x01:
		return "direct mapped";
	case 0xFF:
		return "fully associative";
	default:
		sprintf(buffer, "%d-way associative", assoc);
		return buffer;
	}
}

/* EAX = 8000 0005 */
void handle_ext_amdl1cachefeat(struct cpu_regs_t *regs, __unused struct cpuid_state_t *state)
{
	char buffer[20];
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

	/* This is an AMD-only leaf. */
	if ((state->vendor & VENDOR_AMD) == 0)
		return;

	tlb = (struct amd_l1_tlb_t *)&regs->eax;
	printf("L1 TLBs:\n");

	if (tlb->dtlb_ent)
		printf("  Data TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->dtlb_ent, amd_associativity(buffer, tlb->dtlb_assoc));
	if (tlb->itlb_ent)
		printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->itlb_ent, amd_associativity(buffer, tlb->itlb_assoc));

	tlb = (struct amd_l1_tlb_t *)&regs->ebx;
	if (tlb->dtlb_ent)
		printf("  Data TLB (4KB pages): %d entries, %s\n",
		       tlb->dtlb_ent, amd_associativity(buffer, tlb->dtlb_assoc));
	if (tlb->itlb_ent)
		printf("  Instruction TLB (4KB pages): %d entries, %s\n",
		       tlb->itlb_ent, amd_associativity(buffer, tlb->itlb_assoc));

	printf("\n");

	cache = (struct amd_l1_cache_t *)&regs->ecx;
	if (cache->size)
		printf("L1 caches:\n"
		       "  Data: %dKB, %s, %d lines per tag, %d byte line size\n",
		       cache->size,
		       amd_associativity(buffer, cache->assoc),
		       cache->linespertag,
		       cache->linesize);

	cache = (struct amd_l1_cache_t *)&regs->edx;
	if (cache->size)
		printf("  Instruction: %dKB, %s, %d lines per tag, %d byte line size\n",
		       cache->size,
		       amd_associativity(buffer, cache->assoc),
		       cache->linespertag,
		       cache->linesize);

	printf("\n");
}

/* EAX = 8000 0006 */
void handle_ext_l2cachefeat(struct cpu_regs_t *regs, __unused struct cpuid_state_t *state)
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
			/* 0x0A */ "32-way",
			/* 0x0B */ "48-way",
			/* 0x0C */ "64-way",
			/* 0x0D */ "96-way",
			/* 0x0E */ "128-way",
			/* 0x0F */ "Fully associative"
		};
		char buffer[20];

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

		printf("L2 TLBs:\n");

		tlb = (struct l2_tlb_t *)&regs->eax;
		if (tlb->dtlb_size)
			printf("  Data TLB (2MB and 4MB pages): %d entries, %s\n",
			       tlb->dtlb_size, amd_associativity(buffer, tlb->dtlb_assoc));
		if (tlb->itlb_size)
			printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
			       tlb->itlb_size, amd_associativity(buffer, tlb->itlb_assoc));

		tlb = (struct l2_tlb_t *)&regs->ebx;
		if (tlb->dtlb_size)
			printf("  Data TLB (4KB pages): %d entries, %s\n",
			       tlb->dtlb_size, amd_associativity(buffer, tlb->dtlb_assoc));
		if (tlb->itlb_size)
			printf("  Instruction TLB (4KB pages): %d entries, %s\n",
			       tlb->itlb_size, amd_associativity(buffer, tlb->itlb_assoc));

		printf("\n");

		l2_cache = (struct l2_cache_t *)&regs->ecx;
		if (l2_cache->size)
			printf("L2 cache: %d%cB, %s, %d lines per tag, %d byte line size\n",
			       l2_cache->size > 1024 ? l2_cache->size / 1024 : l2_cache->size,
			       l2_cache->size > 1024 ? 'M' : 'K',
			       assoc[l2_cache->assoc] ? assoc[l2_cache->assoc] : "unknown associativity",
			       l2_cache->linespertag,
			       l2_cache->linesize);

		l3_cache = (struct l3_cache_t *)&regs->edx;
		if (l3_cache->size) {
			uint32_t size = l3_cache->size * 512;
			printf("L3 cache: %u%cB, %s, %d lines per tag, %d byte line size\n",
			       size > 1024 ? size / 1024 : size,
			       size > 1024 ? 'M' : 'K',
			       assoc[l3_cache->assoc] ? assoc[l3_cache->assoc] : "unknown associativity",
			       l3_cache->linespertag,
			       l3_cache->linesize);
		}
		printf("\n");
	}
}

/* EAX = 8000 0007 */
void handle_ext_0007(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->vendor & VENDOR_INTEL) {
		/* TSC information */

		/* Bit 8 of EDX indicates whether the Invariant TSC is available */
		printf("Invariant TSC available: %s\n\n", (regs->edx & 0x100) ? "Yes" : "No");
	}
	if (state->vendor & VENDOR_AMD) {
		/* Advanced Power Management information */

		struct edx_apm_amd_t {
			unsigned ts:1;
			unsigned fid:1;
			unsigned vid:1;
			unsigned ttp:1;
			unsigned tm:1;
			unsigned reserved_1:1;
			unsigned mult100mhz:1;
			unsigned hwpstate:1;
			unsigned tscinvariant:1;
			unsigned cpb:1;
			unsigned efreqro:1;
			unsigned reserved_2:21;
		};
		struct edx_apm_amd_t *edx = (struct edx_apm_amd_t *)&regs->edx;
		if (!regs->edx)
			return;
		printf("AMD Advanced Power Management features:\n");
		if (edx->ts)
			printf("  Temperature Sensor\n");
		if (edx->fid)
			printf("  Frequency ID Control\n");
		if (edx->vid)
			printf("  Voltage ID Control\n");
		if (edx->ttp)
			printf("  THERMTRIP\n");
		if (edx->tm)
			printf("  Hardware thermal control\n");
		if (edx->mult100mhz)
			printf("  100 MHz multiplier Control\n");
		if (edx->hwpstate)
			printf("  Hardware P-state control\n");
		if (edx->tscinvariant)
			printf("  Invariant TSC\n");
		if (edx->cpb)
			printf("  Core performance boost\n");
		if (edx->efreqro)
			printf("  Read-only effective frequency interface\n");
		printf("\n");
	}
}

/* EAX = 8000 0008 */
void handle_ext_0008(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	/* Long mode address size information */
	struct eax_addrsize {
		unsigned physical:8;
		unsigned linear:8;
		unsigned reserved:16;
	};

	struct eax_addrsize *eax = (struct eax_addrsize *)&regs->eax;

	if ((state->vendor & (VENDOR_INTEL | VENDOR_AMD)) == 0)
		return;

	printf("Physical address size: %d bits\n", eax->physical);
	printf("Linear address size: %d bits\n", eax->linear);
	printf("\n");

	if ((state->vendor & VENDOR_AMD) != 0) {
		struct ecx_apiccore {
			unsigned nc:8;
			unsigned reserved_1:4;
			unsigned apicidcoreidsize:4;
			unsigned reserved_2:16;
		};

		struct ecx_apiccore *ecx = (struct ecx_apiccore *)&regs->ecx;

		uint32_t nc = ecx->nc + 1;
		uint32_t mnc = (ecx->apicidcoreidsize > 0) ? (1u << ecx->apicidcoreidsize) : nc;

		printf("Core count: %u\n", nc);
		printf("Maximum core count: %u\n", mnc);
		printf("\n");
	}
}

/* EAX = 8000 000A */
void handle_ext_svm(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct eax_svm {
		unsigned svmrev:8;
		unsigned reserved:24;
	};

	struct ebx_svm {
		unsigned nasid:32;
	};

	struct edx_svm_feat {
		unsigned int mask;
		const char *name;
	} features[] = {
		{0x00000001, "Nested paging"},
		{0x00000002, "LBR virtualization"},
		{0x00000004, "SVM lock"},
		{0x00000008, "NRIP save"},
		{0x00000010, "MSR-based TSC rate control"},
		{0x00000020, "VMCB clean bits"},
		{0x00000040, "Flush by ASID"},
		{0x00000080, "Decode assists"},
		{0x00000400, "Pause intercept filter"},
		{0x00001000, "PAUSE filter threshold"},
		{0x00000000, NULL}
	};

	struct eax_svm *eax = (struct eax_svm *)&regs->eax;
	struct ebx_svm *ebx = (struct ebx_svm *)&regs->ebx;
	struct edx_svm_feat *feat;
	struct cpu_regs_t feat_check;
	unsigned int unaccounted;

	if (!(state->vendor & VENDOR_AMD))
		return;

	/* First check for SVM feature bit. */
	ZERO_REGS(&feat_check);
	feat_check.eax = 0x80000001;
	state->cpuid_call(&feat_check, state);
	if (!(feat_check.ecx & 0x04))
		return;

	printf("SVM Features and Revision Information:\n");
	printf("  Revision: %u\n", eax->svmrev);
	printf("  NASID: %u\n", ebx->nasid);
	printf("  Features:\n");
	unaccounted = 0;
	for (feat = features; feat->mask; feat++) {
		unaccounted |= feat->mask;
		if (regs->edx & feat->mask) {
			printf("    %s\n", feat->name);
		}
	}
	unaccounted = (regs->edx & ~unaccounted);
	if (unaccounted) {
		printf("  Undocumented feature bits: 0x%08x\n", unaccounted);
	}
	printf("\n");
}

/* EAX = 4000 0000 */
void handle_vmm_base(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	char buf[13];

	state->curmax = regs->eax;

	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->ecx;
	*(uint32_t *)(&buf[8]) = regs->edx;
	buf[12] = 0;
	if (strcmp(buf, "XenVMMXenVMM") == 0) {
		state->hypervisor = HYPERVISOR_XEN;
		printf("Xen hypervisor detected\n");
	} else if (strcmp(buf, "VMwareVMware") == 0) {
		state->hypervisor = HYPERVISOR_VMWARE;
		printf("VMware hypervisor detected\n");
	} else if (strcmp(buf, "KVMKVMKVM") == 0) {
		state->hypervisor = HYPERVISOR_KVM;
		printf("KVM hypervisor detected\n");
	} else {
		state->hypervisor = HYPERVISOR_UNKNOWN;
	}
	if (state->hypervisor != HYPERVISOR_UNKNOWN)
		printf("\n");
}

/* EAX = 4000 0001 */
void handle_xen_version(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Xen version: %d.%d\n\n", regs->eax >> 16, regs->eax & 0xFFFF);
}

/* EAX = 4000 0002 */
void handle_xen_leaf02(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Xen features:\n"
	       "  Hypercall transfer pages: %d\n"
	       "  MSR base address: 0x%08x\n\n",
	       regs->eax,
	       regs->ebx);
}

/* EAX = 4000 0003 */
void handle_xen_leaf03(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Host CPU clock frequency: %dMHz\n\n", regs->eax / 1000);
}

/* EAX = 4000 0010 */
void handle_vmware_leaf10(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_VMWARE)
		return;
	printf("TSC frequency: %4.2fMHz\n"
	       "Bus (local APIC timer) frequency: %4.2fMHz\n\n",
	       (float)regs->eax / 1000.0f,
		   (float)regs->ebx / 1000.0f);
}
