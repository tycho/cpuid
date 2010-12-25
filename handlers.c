
#include "prefix.h"
#include "cache.h"
#include "state.h"
#include "handlers.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void handle_features(cpu_regs_t *regs, cpuid_state_t *state);

void handle_std_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_cache02(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_cache04(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_x2apic(cpu_regs_t *regs, cpuid_state_t *state);

void handle_ext_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_pname(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_amdl1cachefeat(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_l2cachefeat(cpu_regs_t *regs, cpuid_state_t *state);

void handle_vmm_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_xen_version(cpu_regs_t *regs, cpuid_state_t *state);
void handle_xen_leaf02(cpu_regs_t *regs, cpuid_state_t *state);
void handle_xen_leaf03(cpu_regs_t *regs, cpuid_state_t *state);

void handle_dump_std_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_dump_std_04(cpu_regs_t *regs, cpuid_state_t *state);
void handle_dump_std_0B(cpu_regs_t *regs, cpuid_state_t *state);

void handle_dump_ext_base(cpu_regs_t *regs, cpuid_state_t *state);

void handle_dump_vmm_base(cpu_regs_t *regs, cpuid_state_t *state);

cpuid_leaf_handler_t std_handlers[] =
{
	handle_std_base, /* 00 */
	handle_features, /* 01 */
	handle_std_cache02, /* 02 */
	NULL, /* 03 */
	handle_std_cache04, /* 04 */
	NULL, /* 05 */
	NULL, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	handle_std_x2apic, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

cpuid_leaf_handler_t ext_handlers[] =
{
	handle_ext_base, /* 00 */
	handle_features, /* 01 */
	handle_ext_pname, /* 02 */
	handle_ext_pname, /* 03 */
	handle_ext_pname, /* 04 */
	handle_ext_amdl1cachefeat, /* 05 */
	handle_ext_l2cachefeat, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	NULL, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

cpuid_leaf_handler_t vmm_handlers[] =
{
	handle_vmm_base, /* 00 */
	handle_xen_version, /* 01 */
	handle_xen_leaf02, /* 02 */
	handle_xen_leaf03, /* 03 */
	NULL, /* 04 */
	NULL, /* 05 */
	NULL, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	NULL, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

cpuid_leaf_handler_t std_dump_handlers[] =
{
	handle_dump_std_base, /* 00 */
	NULL, /* 01 */
	NULL, /* 02 */
	NULL, /* 03 */
	handle_dump_std_04, /* 04 */
	NULL, /* 05 */
	NULL, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	handle_dump_std_0B, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

cpuid_leaf_handler_t ext_dump_handlers[] =
{
	handle_dump_ext_base, /* 00 */
	NULL, /* 01 */
	NULL, /* 02 */
	NULL, /* 03 */
	NULL, /* 04 */
	NULL, /* 05 */
	NULL, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	NULL, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

cpuid_leaf_handler_t vmm_dump_handlers[] =
{
	handle_dump_vmm_base, /* 00 */
	NULL, /* 01 */
	NULL, /* 02 */
	NULL, /* 03 */
	NULL, /* 04 */
	NULL, /* 05 */
	NULL, /* 06 */
	NULL, /* 07 */
	NULL, /* 08 */
	NULL, /* 09 */
	NULL, /* 0A */
	NULL, /* 0B */
	NULL, /* 0C */
	NULL, /* 0D */
	NULL, /* 0E */
	NULL  /* 0F */
};

/* EAX = 0000 0000 */
void handle_dump_std_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	handle_std_base(regs, state);
	printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
		state->last_leaf.eax,
		regs->eax,
		regs->ebx,
		regs->ecx,
		regs->edx,
		reg_to_str(regs));
}

/* EAX = 0000 0000 */
void handle_std_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	char buf[13];
	state->stdmax = regs->eax;
	*(uint32_t *)(&buf[0]) = regs->ebx;
	*(uint32_t *)(&buf[4]) = regs->edx;
	*(uint32_t *)(&buf[8]) = regs->ecx;
	buf[12] = 0;
	if (strcmp(buf, "GenuineIntel") == 0)
		state->vendor = VENDOR_INTEL;
	else if (strcmp(buf, "AuthenticAMD") == 0)
		state->vendor = VENDOR_AMD;
	else
		state->vendor = VENDOR_UNKNOWN;
}

/* EAX = 8000 0001 | EAX = 0000 0001 */
void handle_features(cpu_regs_t *regs, cpuid_state_t *state)
{
	if (state->last_leaf.eax == 0x00000001) {
		typedef struct {
			uint8_t brandid;
			uint8_t clflushsz;
			uint8_t logicalcount;
			uint8_t localapicid;
		} std1_ebx_t;
		std1_ebx_t *ebx = (std1_ebx_t *)&regs->ebx;
		*(uint32_t *)(&state->sig) = regs->eax;
		printf("Signature: 0x%08x\n"
		       "  Family: %d\n"
		       "  Model: %d\n"
		       "  Stepping: %d\n\n",
			*(uint32_t *)&state->sig,
			state->sig.family + state->sig.extfamily,
			state->sig.model + (state->sig.extmodel << 4),
			state->sig.stepping);
		printf("Local APIC: %d\n"
		       "Logical processor count: %d\n"
		       "CLFLUSH size: %d\n"
		       "Brand ID: %d\n\n",
		       ebx->localapicid,
		       ebx->logicalcount,
		       ebx->clflushsz,
		       ebx->brandid);
	}
	print_features(regs, state);
	printf("\n");
}

/* EAX = 0000 0002 */
void handle_std_cache02(cpu_regs_t *regs, cpuid_state_t *state)
{
	uint8_t i, m = regs->eax & 0xFF;
	if (state->vendor != VENDOR_INTEL)
		return;
	printf("Cache descriptors:\n");
	print_intel_caches(regs, &state->sig);
	for (i = 1; i < m; i++) {
		ZERO_REGS(regs);
		regs->eax = 2;
		cpuid_native(regs, state);
		print_intel_caches(regs, &state->sig);
	}
	printf("\n");
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
void handle_std_cache04(cpu_regs_t *regs, cpuid_state_t *state)
{
#pragma pack(push,1)
	typedef struct {
		uint8_t type:5;
		uint8_t level:3;
		uint8_t self_initializing:1;
		uint8_t fully_associative:1;
		uint8_t reserved:4;
		uint16_t max_threads_sharing:12; /* +1 encoded */
		uint8_t apics_reserved:6; /* +1 encoded */
	} eax_cache04_t;
	typedef struct {
		uint16_t line_size:12; /* +1 encoded */
		uint16_t partitions:10; /* +1 encoded */
		uint16_t assoc:10; /* +1 encoded */
	} ebx_cache04_t;
#pragma pack(pop)
	uint32_t i = 0;
	if (state->vendor != VENDOR_INTEL)
		return;
	printf("Deterministic Cache Parameters:\n");
	if (sizeof(eax_cache04_t) != 4 || sizeof(ebx_cache04_t) != 4) {
		printf("  WARNING: The code appears to have been incorrectly compiled.\n"
		       "           Expect wildly inaccurate output for this section.\n");
	}

	while (1) {
		eax_cache04_t *eax;
		ebx_cache04_t *ebx;
		uint32_t cacheSize;
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		cpuid_native(regs, state);

		/* This is a non-official check. With other leafs (i.e. 0x0B),
		   some extra information comes through, past the termination
		   condition. I want to show all the information the CPU provides,
		   even if it's not specified by the Intel docs. */
		if (!regs->eax && !regs->ebx && !regs->ecx && !regs->edx)
			break;

		eax = (eax_cache04_t *)&regs->eax;
		ebx = (ebx_cache04_t *)&regs->ebx;

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
void handle_dump_std_04(cpu_regs_t *regs, cpuid_state_t *state)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		cpuid_native(regs, state);
		printf("CPUID %08x, index %d = %08x %08x %08x %08x | %s\n",
			state->last_leaf.eax,
			state->last_leaf.ecx,
			regs->eax,
			regs->ebx,
			regs->ecx,
			regs->edx,
			reg_to_str(regs));
		if (!(regs->eax & 0xF))
			break;
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
void handle_std_x2apic(cpu_regs_t *regs, cpuid_state_t *state)
{
	uint32_t i = 0;
	printf("Processor Topology:\n");
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		cpuid_native(regs, state);
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
void handle_dump_std_0B(cpu_regs_t *regs, cpuid_state_t *state)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		cpuid_native(regs, state);
		printf("CPUID %08x, index %d = %08x %08x %08x %08x | %s\n",
			state->last_leaf.eax,
			state->last_leaf.ecx,
			regs->eax,
			regs->ebx,
			regs->ecx,
			regs->edx,
			reg_to_str(regs));
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
}

/* EAX = 8000 0000 */
void handle_dump_ext_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	handle_ext_base(regs, state);
	printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
		state->last_leaf.eax,
		regs->eax,
		regs->ebx,
		regs->ecx,
		regs->edx,
		reg_to_str(regs));
}

/* EAX = 8000 0000 */
void handle_ext_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	state->extmax = regs->eax;
}

/* EAX = 8000 0002 */
void handle_ext_pname(cpu_regs_t *regs, cpuid_state_t *state)
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

static const char *amd_associativity(uint8_t assoc)
{
	static char buf[20];
	switch (assoc) {
	case 0x00:
		return "Reserved";
	case 0x01:
		return "direct mapped";
	case 0xFF:
		return "fully associative";
	default:
		sprintf(buf, "%d-way associative", assoc);
		return buf;
	}
}

/* EAX = 8000 0005 */
void handle_ext_amdl1cachefeat(cpu_regs_t *regs, unused cpuid_state_t *state)
{
	typedef struct {
		uint8_t itlb_ent;
		uint8_t itlb_assoc;
		uint8_t dtlb_ent;
		uint8_t dtlb_assoc;
	} amd_l1_tlb_t;
	typedef struct {
		uint8_t linesize;
		uint8_t linespertag;
		uint8_t assoc;
		uint8_t size;
	} amd_l1_cache_t;
	amd_l1_tlb_t *tlb;
	amd_l1_cache_t *cache;

	/* This is an AMD-only leaf. */
	if (state->vendor != VENDOR_AMD)
		return;

	tlb = (amd_l1_tlb_t *)&regs->eax;
	printf("L1 TLBs:\n");

	if (tlb->dtlb_ent)
		printf("  Data TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->dtlb_ent, amd_associativity(tlb->dtlb_assoc));
	if (tlb->itlb_ent)
		printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->itlb_ent, amd_associativity(tlb->itlb_assoc));

	tlb = (amd_l1_tlb_t *)&regs->ebx;
	if (tlb->dtlb_ent)
		printf("  Data TLB (4KB pages): %d entries, %s\n",
		       tlb->dtlb_ent, amd_associativity(tlb->dtlb_assoc));
	if (tlb->itlb_ent)
		printf("  Instruction TLB (4KB pages): %d entries, %s\n",
		       tlb->itlb_ent, amd_associativity(tlb->itlb_assoc));

	printf("\n");

	cache = (amd_l1_cache_t *)&regs->ecx;
	if (cache->size)
		printf("L1 caches:\n"
		       "  Data: %dKB, %s, %d lines per tag, %d byte line size\n",
		       cache->size,
		       amd_associativity(cache->assoc),
		       cache->linespertag,
		       cache->linesize);

	cache = (amd_l1_cache_t *)&regs->edx;
	if (cache->size)
		printf("  Instruction: %dKB, %s, %d lines per tag, %d byte line size\n",
		       cache->size,
		       amd_associativity(cache->assoc),
		       cache->linespertag,
		       cache->linesize);

	printf("\n");
}

/* EAX = 8000 0006 */
void handle_ext_l2cachefeat(cpu_regs_t *regs, unused cpuid_state_t *state)
{
	if (state->vendor == VENDOR_INTEL) {
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

		typedef struct {
			uint8_t linesize;
			uint8_t reserved1:4;
			uint8_t assoc:4;
			uint16_t size;
		} l2cache_feat_t;

		l2cache_feat_t *feat = (l2cache_feat_t *)&regs->ecx;

		printf("L2 cache:\n"
		       "  %d%cB, %s associativity, %d byte line size\n\n",
			feat->size > 1024 ? feat->size / 1024 : feat->size,
			feat->size > 1024 ? 'M' : 'K',
			assoc[feat->assoc] ? assoc[feat->assoc] : "Unknown",
			feat->linesize);
	}

	if (state->vendor == VENDOR_AMD) {
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

		typedef struct {
			uint16_t itlb_size:12;
			uint8_t  itlb_assoc:4;
			uint16_t dtlb_size:12;
			uint8_t  dtlb_assoc:4;
		} l2_tlb_t;
		typedef struct {
			uint8_t  linesize;
			uint8_t  linespertag:4;
			uint8_t  assoc:4;
			uint16_t size;
		} l2_cache_t;
		typedef struct {
			uint8_t  linesize;
			uint8_t  linespertag:4;
			uint8_t  reserved:2;
			uint16_t size:14;
		} l3_cache_t;

		l2_tlb_t *tlb;
		l2_cache_t *l2_cache;
		l3_cache_t *l3_cache;

		printf("L2 TLBs:\n");

		tlb = (l2_tlb_t *)&regs->eax;
		if (tlb->dtlb_size)
			printf("  Data TLB (2MB and 4MB pages): %d entries, %s\n",
			       tlb->dtlb_size, amd_associativity(tlb->dtlb_assoc));
		if (tlb->itlb_size)
			printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
			       tlb->itlb_size, amd_associativity(tlb->itlb_assoc));

		tlb = (l2_tlb_t *)&regs->ebx;
		if (tlb->dtlb_size)
			printf("  Data TLB (4KB pages): %d entries, %s\n",
			       tlb->dtlb_size, amd_associativity(tlb->dtlb_assoc));
		if (tlb->itlb_size)
			printf("  Instruction TLB (4KB pages): %d entries, %s\n",
			       tlb->itlb_size, amd_associativity(tlb->itlb_assoc));

		printf("\n");

		l2_cache = (l2_cache_t *)&regs->ecx;
		if (l2_cache->size)
			printf("L2 cache: %d%cB, %s, %d lines per tag, %d byte line size\n",
			       l2_cache->size > 1024 ? l2_cache->size / 1024 : l2_cache->size,
			       l2_cache->size > 1024 ? 'M' : 'K',
			       assoc[l2_cache->assoc] ? assoc[l2_cache->assoc] : "unknown associativity",
			       l2_cache->linespertag,
			       l2_cache->linesize);

		l3_cache = (l3_cache_t *)&regs->edx;
		if (l3_cache->size)
			printf("L3 cache: %d%cB, %d lines per tag, %d byte line size\n",
			       l3_cache->size > 1024 ? l3_cache->size / 1024 : l3_cache->size,
			       l3_cache->size > 1024 ? 'M' : 'K',
			       l3_cache->linespertag,
			       l3_cache->linesize);

		printf("\n");
	}
}

/* EAX = 4000 0000 */
void handle_dump_vmm_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	state->hvmax = regs->eax;
	printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
		state->last_leaf.eax,
		regs->eax,
		regs->ebx,
		regs->ecx,
		regs->edx,
		reg_to_str(regs));
}

/* EAX = 4000 0000 */
void handle_vmm_base(cpu_regs_t *regs, cpuid_state_t *state)
{
	char buf[13];
	state->hvmax = regs->eax;
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
	} else {
		state->hypervisor = HYPERVISOR_UNKNOWN;
	}
	printf("\n");
}

/* EAX = 4000 0001 */
void handle_xen_version(cpu_regs_t *regs, cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Xen version: %d.%d\n\n", regs->eax >> 16, regs->eax & 0xFFFF);
}

/* EAX = 4000 0002 */
void handle_xen_leaf02(cpu_regs_t *regs, cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Xen features:\n"
	       "  Hypercall transfer pages: %d\n"
	       "  MSR base address: %08x\n\n",
	       regs->eax,
	       regs->ebx);
}

/* EAX = 4000 0003 */
void handle_xen_leaf03(cpu_regs_t *regs, cpuid_state_t *state)
{
	if (state->hypervisor != HYPERVISOR_XEN)
		return;
	printf("Host CPU clock frequency: %dMHz\n\n", regs->eax / 1000);
}
