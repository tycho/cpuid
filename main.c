#include "prefix.h"
#include "cpuid.h"
#include "vendor.h"
#include "feature.h"
#include "cache.h"
#include "util.h"
#include "state.h"
#include "handlers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void dump_cpuid(cpuid_state_t *state)
{
	uint32_t i;
	cpu_regs_t cr_tmp;

	for (i = 0; i <= state->stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(std_dump_handlers, i))
			std_dump_handlers[i](&cr_tmp, state);
		else
			printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
				state->last_leaf.eax,
				cr_tmp.eax,
				cr_tmp.ebx,
				cr_tmp.ecx,
				cr_tmp.edx,
				reg_to_str(&cr_tmp));
	}
	
	for (i = 0x80000000; i <= state->extmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(ext_dump_handlers, i - 0x80000000))
			ext_dump_handlers[i - 0x80000000](&cr_tmp, state);
		else
			printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
				state->last_leaf.eax,
				cr_tmp.eax,
				cr_tmp.ebx,
				cr_tmp.ecx,
				cr_tmp.edx,
				reg_to_str(&cr_tmp));
	}

	for (i = 0x40000000; i <= state->hvmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(vmm_dump_handlers, i - 0x40000000))
			vmm_dump_handlers[i - 0x40000000](&cr_tmp, state);
		else
			printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
				state->last_leaf.eax,
				cr_tmp.eax,
				cr_tmp.ebx,
				cr_tmp.ecx,
				cr_tmp.edx,
				reg_to_str(&cr_tmp));
	}
	printf("\n");
}

void run_cpuid(cpuid_state_t *state)
{
	uint32_t i;
	cpu_regs_t cr_tmp;

	for (i = 0; i <= state->stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(std_handlers, i))
			std_handlers[i](&cr_tmp, state);
	}
	
	for (i = 0x80000000; i <= state->extmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(ext_handlers, i - 0x80000000))
			ext_handlers[i - 0x80000000](&cr_tmp, state);
	}

	for (i = 0x40000000; i <= state->hvmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		cpuid_native(&cr_tmp, state);
		if (HAS_HANDLER(vmm_handlers, i - 0x40000000))
			vmm_handlers[i - 0x40000000](&cr_tmp, state);
	}
}

int main(unused int argc, unused char **argv)
{
	cpuid_state_t state;
	INIT_CPUID_STATE(&state);
	dump_cpuid(&state);
	INIT_CPUID_STATE(&state);
	run_cpuid(&state);
	return 0;
}
