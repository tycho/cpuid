
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
	NULL, /* 05 */
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

/* EAX = 8000 0006 */
void handle_ext_l2cachefeat(cpu_regs_t *regs, unused cpuid_state_t *state)
{
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
