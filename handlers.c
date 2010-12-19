
#include "prefix.h"
#include "cache.h"
#include "state.h"
#include "handlers.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void handle_features(cpu_regs_t *regs, cpuid_state_t *state);

void handle_std_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_cache(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_x2apic(cpu_regs_t *regs, cpuid_state_t *state);

void handle_ext_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_pname(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_amdl1cachefeat(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_l2cachefeat(cpu_regs_t *regs, cpuid_state_t *state);

void handle_dump_std_04(cpu_regs_t *regs, cpuid_state_t *state);
void handle_dump_std_0B(cpu_regs_t *regs, cpuid_state_t *state);

cpu_std_handler std_handlers[] =
{
	handle_std_base, /* 00 */
	handle_features, /* 01 */
	handle_std_cache, /* 02 */
	NULL, /* 03 */
	NULL, /* 04 */
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

cpu_std_handler ext_handlers[] =
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

cpu_std_handler std_dump_handlers[] =
{
	NULL, /* 00 */
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

cpu_std_handler ext_dump_handlers[] =
{
	NULL, /* 00 */
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
		*(uint32_t *)(&state->sig) = regs->eax;
		printf("Signature: 0x%08x\n"
		       "Family: %d\n"
		       "Model: %d\n"
		       "Stepping: %d\n\n",
			*(uint32_t *)&state->sig,
			state->sig.family + state->sig.extfamily,
			state->sig.model + (state->sig.extmodel << 4),
			state->sig.stepping);
	}
	print_features(regs, state->last_leaf.eax, state->vendor);
	printf("\n");
}

/* EAX = 0000 0002 */
void handle_std_cache(cpu_regs_t *regs, cpuid_state_t *state)
{
	uint8_t i, m = regs->eax & 0xFF;
	printf("Cache descriptors:\n");
	print_caches(regs, &state->sig);
	for (i = 1; i < m; i++) {
		ZERO_REGS(regs);
		regs->eax = 2;
		cpuid_native(regs, state);
		print_caches(regs, &state->sig);
	}
	printf("\n");
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
		uint8_t dtlb_assoc;
		uint8_t dtlb_ent;
		uint8_t itlb_assoc;
		uint8_t itlb_ent;
	} amd_l1_tlb_t;
	typedef struct {
		uint8_t size;
		uint8_t assoc;
		uint8_t linespertag;
		uint8_t linesize;
	} amd_l1_cache_t;
	amd_l1_tlb_t *tlb;
	amd_l1_cache_t *cache;

	/* This is an AMD-only leaf. */
	if (state->vendor != VENDOR_AMD)
		return;

	tlb = (amd_l1_tlb_t *)&regs->eax;
	printf("L1 TLBs:\n"
	       "  Data TLB (2MB and 4MB pages): %d entries, %s\n",
	       tlb->dtlb_ent, amd_associativity(tlb->dtlb_assoc));
	printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
	       tlb->itlb_ent, amd_associativity(tlb->itlb_assoc));

	tlb = (amd_l1_tlb_t *)&regs->ebx;
	printf("  Data TLB (4KB pages): %d entries, %s\n",
	       tlb->dtlb_ent, amd_associativity(tlb->dtlb_assoc));
	printf("  Instruction TLB (4KB pages): %d entries, %s\n",
	       tlb->itlb_ent, amd_associativity(tlb->itlb_assoc));

	cache = (amd_l1_cache_t *)&regs->ecx;
	printf("L1 caches:\n"
	       "  Data: %dKB, %s, %d lines per tag, %d byte line size\n",
	       cache->size,
	       amd_associativity(cache->assoc),
	       cache->linespertag,
	       cache->linesize);

	cache = (amd_l1_cache_t *)&regs->edx;
	printf("  Instruction: %dKB, %s, %d lines per tag, %d byte line size\n",
	       cache->size,
	       amd_associativity(cache->assoc),
	       cache->linespertag,
	       cache->linesize);
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
			uint8_t  dtlb_assoc:4;
			uint16_t dtlb_size:12;
			uint8_t  itlb_assoc:4;
			uint16_t itlb_size:12;
		} l2_tlb_t;
		typedef struct {
			uint16_t size;
			uint8_t  assoc:4;
			uint8_t  linespertag:4;
			uint8_t  linesize;
		} l2_cache_t;
		typedef struct {
			uint16_t size:14;
			uint8_t  reserved:2;
			uint8_t  linespertag:4;
			uint8_t  linesize;
		} l3_cache_t;

		l2_tlb_t *tlb;
		l2_cache_t *l2_cache;
		l3_cache_t *l3_cache;

		tlb = (l2_tlb_t *)&regs->eax;
		printf("L2 TLBs:\n"
		       "  Data TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->dtlb_size, amd_associativity(tlb->dtlb_assoc));
		printf("  Instruction TLB (2MB and 4MB pages): %d entries, %s\n",
		       tlb->itlb_size, amd_associativity(tlb->itlb_assoc));

		tlb = (l2_tlb_t *)&regs->ebx;
		printf("  Data TLB (4KB pages): %d entries, %s\n",
		       tlb->dtlb_size, amd_associativity(tlb->dtlb_assoc));
		printf("  Instruction TLB (4KB pages): %d entries, %s\n",
		       tlb->itlb_size, amd_associativity(tlb->itlb_assoc));

		l2_cache = (l2_cache_t *)&regs->ecx;
		printf("L2 cache: %dKB, %s, %d lines per tag, %d byte line size\n",
		       l2_cache->size,
		       assoc[l2_cache->assoc] ? assoc[l2_cache->assoc] : "unknown associativity",
		       l2_cache->linespertag,
		       l2_cache->linesize);

		l3_cache = (l3_cache_t *)&regs->edx;
		printf("L3 cache: %dKB, %d lines per tag, %d byte line size\n",
		       l3_cache->size,
		       l3_cache->linespertag,
		       l3_cache->linesize);
	}
}
