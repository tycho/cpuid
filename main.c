#include "prefix.h"

#include "cache.h"
#include "cpuid.h"
#include "feature.h"
#include "handlers.h"
#include "util.h"
#include "state.h"
#include "vendor.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

int ignore_vendor = 0;

void dump_cpuid(struct cpuid_state_t *state)
{
	uint32_t i;
	struct cpu_regs_t cr_tmp;

	for (i = 0; i <= state->stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
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
		state->cpuid_call(&cr_tmp, state);
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
		state->cpuid_call(&cr_tmp, state);
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

void run_cpuid(struct cpuid_state_t *state)
{
	uint32_t i;
	struct cpu_regs_t cr_tmp;

	for (i = 0; i <= state->stdmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(std_handlers, i))
			std_handlers[i](&cr_tmp, state);
	}
	
	for (i = 0x80000000; i <= state->extmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(ext_handlers, i - 0x80000000))
			ext_handlers[i - 0x80000000](&cr_tmp, state);
	}

	for (i = 0x40000000; i <= state->hvmax; i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(vmm_handlers, i - 0x40000000))
			vmm_handlers[i - 0x40000000](&cr_tmp, state);
	}
}

void usage(const char *argv0)
{
	printf("usage: %s [--help] [--dump] [--ignore-vendor] [--parse <filename>]\n\n", argv0);
	printf("  %-18s %s\n", "-h, --help", "Print this list of options");
	printf("  %-18s %s\n", "-d, --dump", "Dump a raw CPUID table");
	printf("  %-18s %s\n", "--ignore-vendor", "Show feature flags from all vendors");
	printf("  %-18s %s\n", "-f, --parse", "Read and decode a raw cpuid table from the file specified");
	printf("\n");
	exit(0);
}

void version()
{
	printf("cpuid version %s\n\n", cpuid_version_long());
	license();
	exit(0);
}

static int do_dump = 0;

int main(int argc, char **argv)
{
	const char *file = NULL;
	struct cpuid_state_t state;
	int c;

	while (TRUE) {
		static struct option long_options[] = {
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{"dump", no_argument, &do_dump, 1},
			{"ignore-vendor", no_argument, &ignore_vendor, 1},
			{"parse", required_argument, 0, 'f'},
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "hdvf:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 0:
			break;
		case 'd':
			do_dump = 1;
			break;
		case 'f':
			file = optarg;
			break;
		case 'v':
			version();
		case 'h':
		case '?':
		default:
			usage(argv[0]);
		}
	}

	if (do_dump) {
		INIT_CPUID_STATE(&state);
		if (file) {
			cpuid_load_from_file(file, &state);
			state.cpuid_call = cpuid_pseudo;
		}
		dump_cpuid(&state);
		FREE_CPUID_STATE(&state);
	} else {
		INIT_CPUID_STATE(&state);
		if (file) {
			cpuid_load_from_file(file, &state);
			state.cpuid_call = cpuid_pseudo;
		}
		run_cpuid(&state);
		FREE_CPUID_STATE(&state);
	}

	return 0;
}
