#include "prefix.h"

#include "cache.h"
#include "cpuid.h"
#include "feature.h"
#include "handlers.h"
#include "util.h"
#include "state.h"
#include "vendor.h"
#include "version.h"

#ifdef TARGET_OS_LINUX

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#define CPUSET_T cpu_set_t

#elif defined(TARGET_OS_WINDOWS)

#include <windows.h>

#elif defined(TARGET_OS_FREEBSD)

#include <pthread_np.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#define CPUSET_T cpuset_t

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

int ignore_vendor = 0;
uint32_t scan_to = 0;

void dump_cpuid(struct cpuid_state_t *state)
{
	uint32_t i;
	struct cpu_regs_t cr_tmp;

	for (i = 0; i <= (scan_to ? scan_to : state->stdmax); i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(std_dump_handlers, i))
			std_dump_handlers[i](&cr_tmp, state);
		else
			state->cpuid_print(&cr_tmp, state, FALSE);
	}
	
	for (i = 0x80000000; i <= (scan_to ? 0x80000000 + scan_to : state->extmax); i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(ext_dump_handlers, i - 0x80000000))
			ext_dump_handlers[i - 0x80000000](&cr_tmp, state);
		else
			state->cpuid_print(&cr_tmp, state, FALSE);
	}

	for (i = 0x40000000; i <= (scan_to ? 0x40000000 + scan_to : state->hvmax); i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(vmm_dump_handlers, i - 0x40000000))
			vmm_dump_handlers[i - 0x40000000](&cr_tmp, state);
		else
			state->cpuid_print(&cr_tmp, state, FALSE);
	}
	printf("\n");
}

void run_cpuid(struct cpuid_state_t *state)
{
	uint32_t i;
	struct cpu_regs_t cr_tmp;

	for (i = 0; i <= (scan_to ? scan_to : state->stdmax); i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(std_handlers, i))
			std_handlers[i](&cr_tmp, state);
	}
	
	for (i = 0x80000000; i <= (scan_to ? 0x80000000 + scan_to : state->extmax); i++) {
		ZERO_REGS(&cr_tmp);
		cr_tmp.eax = i;
		state->cpuid_call(&cr_tmp, state);
		if (HAS_HANDLER(ext_handlers, i - 0x80000000))
			ext_handlers[i - 0x80000000](&cr_tmp, state);
	}

	for (i = 0x40000000; i <= (scan_to ? 0x40000000 + scan_to : state->hvmax); i++) {
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
	printf("  %-18s %s\n", "-c, --cpu", "Index (starting at 0) of CPU to get info from");
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

void set_affinity(uint32_t num)
{
#ifdef TARGET_OS_WINDOWS

	DWORD ret;
	HANDLE hThread = GetCurrentThread();
	ret = SetThreadAffinityMask(hThread, 1 << num);

	if (ret == 0) {
		printf("Could not bind to CPU at index %d. Does it exist?\n", num);
		exit(1);
	}

#elif defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)

	int ret;
	CPUSET_T mask;
	pthread_t pth;

	pth = pthread_self();
	CPU_ZERO(&mask);
	CPU_SET(1 << num, &mask);
	ret = pthread_setaffinity_np(pth, sizeof(mask), &mask);

	if (ret != 0) {
		printf("Could not bind to CPU at index %d. Does it exist?\n", num);
		exit(1);
	}

#endif
}

static int do_dump = 0;
static int dump_vmware = 0;
static int cpu_index = 0;

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
			{"cpu", required_argument, 0, 'c'},
			{"ignore-vendor", no_argument, &ignore_vendor, 1},
			{"parse", required_argument, 0, 'f'},
			{"vmware-vmx", no_argument, &dump_vmware, 1},
			{"scan-to", required_argument, 0, 2},
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "c:hdvf:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 0:
			break;
		case 2:
			assert(optarg);
			if (sscanf(optarg, "0x%x", &scan_to) != 1)
				if (sscanf(optarg, "%u", &scan_to) != 1)
					if (sscanf(optarg, "%x", &scan_to) != 1)
						scan_to = 0;
			break;
		case 'c':
			assert(optarg);
			if (sscanf(optarg, "%d", &cpu_index) != 1) {
				printf("That wasn't an integer.\n");
				exit(1);
			}
			if (cpu_index < 0) {
				printf("Hah. Very funny. Zero or above, please.\n");
				exit(1);
			}
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

	set_affinity(cpu_index);

	if (dump_vmware) {
		/* --vmware-vmx implies --dump */
		do_dump = 1;
		state.cpuid_print = cpuid_dump_vmware;
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
