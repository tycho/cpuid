#include "prefix.h"
#include "cpuid.h"
#include "vendor.h"
#include "feature.h"
#include "cache.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t stdmax = 0;
uint32_t extmax = 0;

cpu_signature_t sig;
cpu_vendor_t vendor = VENDOR_UNKNOWN;
char procname[48];

/* Makes a lot of calls easier to do. */
static inline BOOL cpuid_native(cpu_regs_t *regs) { return cpuid(&regs->eax, &regs->ebx, &regs->ecx, &regs->edx); }

typedef void(*cpu_std_handler)(cpu_regs_t *);

void handle_std_base(cpu_regs_t *regs);
void handle_std_features(cpu_regs_t *regs);
void handle_std_cache(cpu_regs_t *regs);
void handle_std_x2apic(cpu_regs_t *regs);

void handle_ext_base(cpu_regs_t *regs);
void handle_ext_features(cpu_regs_t *regs);
void handle_ext_pname2(cpu_regs_t *regs);
void handle_ext_pname3(cpu_regs_t *regs);
void handle_ext_pname4(cpu_regs_t *regs);

void handle_dump_std_04(cpu_regs_t *regs);
void handle_dump_std_0B(cpu_regs_t *regs);

cpu_std_handler std_handlers[] = 
{
	handle_std_base, /* 00 */
	handle_std_features, /* 01 */
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
	handle_ext_features, /* 01 */
	handle_ext_pname2, /* 02 */
	handle_ext_pname3, /* 03 */
	handle_ext_pname4, /* 04 */
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

#define HAS_HANDLER(handlers, ind) ((ind) < 0x10 && (handlers[(ind)]))

uint32_t get_std_max();
uint32_t get_ext_max();

void dump_cpuid()
{
	uint32_t i;
	cpu_regs_t cr_tmp;
	
	/* Kind of a kludge, but we have to handle the stdmax and
	   extmax before we can do a full dump */
	ZERO_REGS(&cr_tmp);
	cpuid_native(&cr_tmp);
	std_handlers[0](&cr_tmp);

	ZERO_REGS(&cr_tmp);
	cr_tmp.eax = 0x80000000;
	cpuid_native(&cr_tmp);
	ext_handlers[0](&cr_tmp);

	for (i = 0; i <= stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp);
		if (HAS_HANDLER(std_dump_handlers, i))
			std_dump_handlers[i](&cr_tmp);
		else
			printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
				i, cr_tmp.eax, cr_tmp.ebx, cr_tmp.ecx, cr_tmp.edx, reg_to_str(&cr_tmp));
	}
	
	for (i = 0x80000000; i <= extmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp);
		if (HAS_HANDLER(ext_dump_handlers, i - 0x80000000))
			ext_dump_handlers[i - 0x80000000](&cr_tmp);
		else
			printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
				i, cr_tmp.eax, cr_tmp.ebx, cr_tmp.ecx, cr_tmp.edx, reg_to_str(&cr_tmp));
	}
}

void run_cpuid()
{
	uint32_t i;
	cpu_regs_t cr_tmp;
	
	/* Kind of a kludge, but we have to handle the stdmax and
	   extmax before we can do a full dump */
	ZERO_REGS(&cr_tmp);
	cpuid_native(&cr_tmp);
	std_handlers[0](&cr_tmp);

	ZERO_REGS(&cr_tmp);
	cr_tmp.eax = 0x80000000;
	cpuid_native(&cr_tmp);
	ext_handlers[0](&cr_tmp);

	for (i = 0; i <= stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp);
		if (HAS_HANDLER(std_handlers, i))
			std_handlers[i](&cr_tmp);
	}
	
	for (i = 0x80000000; i <= extmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp);
		if (HAS_HANDLER(ext_handlers, i - 0x80000000))
			ext_handlers[i - 0x80000000](&cr_tmp);
	}
}

/* EAX = 0000 0000 */
void handle_std_base(cpu_regs_t *regs)
{
	char buf[13];
	stdmax = regs->eax;
	memcpy(&buf[0], &regs->ebx, sizeof(uint32_t));
	memcpy(&buf[4], &regs->edx, sizeof(uint32_t));
	memcpy(&buf[8], &regs->ecx, sizeof(uint32_t));
	buf[12] = 0;
	if (strcmp(buf, "GenuineIntel") == 0)
		vendor = VENDOR_INTEL;
	else if (strcmp(buf, "AuthenticAMD") == 0)
		vendor = VENDOR_AMD;
	else
		vendor = VENDOR_UNKNOWN;
}

/* EAX = 0000 0001 */
void handle_std_features(cpu_regs_t *regs)
{
	*(uint32_t *)(&sig) = regs->eax;
	printf("Family: %d\nModel: %d\nStepping: %d\n\n",
		sig.family + sig.extfamily, sig.model + (sig.extmodel << 4), sig.stepping);
	print_features(regs, 0x00000001, vendor);
	printf("\n");
}

/* EAX = 0000 0002 */
void handle_std_cache(cpu_regs_t *regs)
{
	uint8_t i, m = regs->eax & 0xFF;
	printf("Cache descriptors:\n");
	print_caches(regs, &sig);
	for (i = 1; i < m; i++) {
		ZERO_REGS(regs);
		regs->eax = 2;
		cpuid_native(regs);
		print_caches(regs, &sig);
	}
	printf("\n");
}

/* EAX = 0000 0004 */
void handle_dump_std_04(cpu_regs_t *regs)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 4;
		regs->ecx = i;
		cpuid_native(regs);
		printf("CPUID %08x, index %d = %08x %08x %08x %08x | %s\n",
			4, i, regs->eax, regs->ebx, regs->ecx, regs->edx, reg_to_str(regs));
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
void handle_std_x2apic(cpu_regs_t *regs)
{
	uint32_t i = 0;
	printf("Processor Topology:\n");
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		cpuid_native(regs);
		printf("  Bits to shift: %d\n  Logical at this level: %d\n  Level number: %d\n  Level type: %d (%s)\n  x2APIC ID: %d\n\n",
			regs->eax & 0x1f, regs->ebx & 0xffff, regs->ecx & 0xff, (regs->ecx >> 8) & 0xff, x2apic_level_type((regs->ecx >> 8) & 0xff), regs->edx );
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
	printf("\n");
}

/* EAX = 0000 000B */
void handle_dump_std_0B(cpu_regs_t *regs)
{
	uint32_t i = 0;
	while (1) {
		ZERO_REGS(regs);
		regs->eax = 0xb;
		regs->ecx = i;
		cpuid_native(regs);
		printf("CPUID %08x, index %d = %08x %08x %08x %08x | %s\n",
			0xB, i, regs->eax, regs->ebx, regs->ecx, regs->edx, reg_to_str(regs));
		if (!(regs->eax || regs->ebx))
			break;
		i++;
	}
}

/* EAX = 8000 0000 */
void handle_ext_base(cpu_regs_t *regs)
{
	extmax = regs->eax;
}

/* EAX = 0000 0001 */
void handle_ext_features(cpu_regs_t *regs)
{
	print_features(regs, 0x80000001, vendor);
	printf("\n");
}

static void squeeze(char *str)
{
	int r; /* next character to be read */
	int w; /* next character to be written */

	r=w=0;
	while (str[r])
	{
		if (isspace(str[r]) || iscntrl(str[r]))
		{
			if (w > 0 && !isspace(str[w-1]))
				str[w++] = ' ';
		}
		else
			str[w++] = str[r];
		r++;
	}
	str[w] = 0;
}

/* EAX = 0000 0002 */
void handle_ext_pname2(cpu_regs_t *regs)
{
	memset(procname, 0, sizeof(procname));
	*(uint32_t *)&procname[0] = regs->eax;
	*(uint32_t *)&procname[4] = regs->ebx;
	*(uint32_t *)&procname[8] = regs->ecx;
	*(uint32_t *)&procname[12] = regs->edx;
}

/* EAX = 0000 0003 */
void handle_ext_pname3(cpu_regs_t *regs)
{
	*(uint32_t *)&procname[16] = regs->eax;
	*(uint32_t *)&procname[20] = regs->ebx;
	*(uint32_t *)&procname[24] = regs->ecx;
	*(uint32_t *)&procname[28] = regs->edx;
}

/* EAX = 0000 0004 */
void handle_ext_pname4(cpu_regs_t *regs)
{
	*(uint32_t *)&procname[32] = regs->eax;
	*(uint32_t *)&procname[36] = regs->ebx;
	*(uint32_t *)&procname[40] = regs->ecx;
	*(uint32_t *)&procname[44] = regs->edx;
	procname[48] = 0;
	squeeze(procname);
	printf("Processor Name: %s\n\n", procname);
}

int main(unused int argc, unused char **argv)
{
	dump_cpuid();
	printf("\n");
	run_cpuid();
	return 0;
}
