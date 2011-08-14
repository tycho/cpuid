#include "prefix.h"

#include "cache.h"
#include "cpuid.h"
#include "feature.h"
#include "handlers.h"
#include "util.h"
#include "sanity.h"
#include "state.h"
#include "threads.h"
#include "vendor.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

int ignore_vendor = 0;
static uint32_t scan_to = 0;

static void run_cpuid(struct cpuid_state_t *state, int dump)
{
	uint32_t i;
	uint32_t r;
	struct cpu_regs_t cr_tmp, ignore;
	const struct cpuid_leaf_handler_index_t *h;


	/* Arbitrary leaf that's probably never ever used. */
	ZERO_REGS(&ignore);
	ignore.eax = 0xBEEFBABE;
	state->cpuid_call(&ignore, state);

	for (r = 0x00000000;; r += 0x00010000) {
		state->curmax = r;
		for (i = r; i <= (scan_to ? r + scan_to : state->curmax); i++) {

			/* If a particular range is unsupported, the processor can report
			 * a really wacky upper boundary. This is a quick sanity check,
			 * since it's very unlikely that any range would have more than
			 * 0xFFFF indices.
			 */
			if ((state->curmax & 0xFFFF0000) != (i & 0xFFFF0000))
				break;

			ZERO_REGS(&cr_tmp);

			/* ECX isn't populated here. It's the job of any leaf handler to
			 * re-call CPUID with the appropriate ECX values.
			 */
			cr_tmp.eax = i;
			state->cpuid_call(&cr_tmp, state);

			/* Typically, if the range is invalid, the CPU gives an obvious
			 * "bogus" result. We try to catch that here.
			 *
			 * We don't compare the last byte of EDX (size - 1) because on
			 * certain very broken OSes (i.e. Mac OS X) there are no APIs to
			 * force threads to be affinitized to one core. This makes the
			 * value of EDX a bit nondeterministic when CPUID is executed.
			 */
			if (i == r && 0 == memcmp(&ignore, &cr_tmp, sizeof(struct cpu_regs_t) - 1))
				break;

			for (h = dump ? dump_handlers : decode_handlers;
			     h->handler;
			     h++)
			{
				if (h->leaf_id == i)
					break;
			}

			if (h->handler)
				h->handler(&cr_tmp, state);
			else if (dump)
				state->cpuid_print(&cr_tmp, state, FALSE);
		}

		/* Terminating condition.
		 * This is an awkward way to terminate the loop, but if we used
		 * r != 0xFFFF0000 as the terminating condition in the outer loop,
		 * then we would effectively skip probing of 0xFFFF0000. So we
		 * turn this into an awkward do/while+for combination.
		 */
		if (r == 0xFFFF0000)
			break;
	}
}

static void usage(const char *argv0)
{
	printf("usage: %s [--help] [--dump] [--ignore-vendor] [--parse <filename>]\n\n", argv0);
	printf("  %-18s %s\n", "-h, --help", "Print this list of options");
	printf("  %-18s %s\n", "-c, --cpu", "Index (starting at 0) of CPU to get info from");
	printf("  %-18s %s\n", "-d, --dump", "Dump a raw CPUID table");
	printf("  %-18s %s\n", "--ignore-vendor", "Show feature flags from all vendors");
	printf("  %-18s %s\n", "-f, --parse", "Read and decode a raw cpuid table from the file specified");
	printf("  %-18s %s\n", "--sanity", "Do a sanity check of the CPUID data");
	printf("\n");
	exit(0);
}

static void version(void)
{
	printf("cpuid version %s\n\n", cpuid_version_long());
	license();
	exit(0);
}

enum {
	DUMP_FORMAT_DEFAULT = 1,
	DUMP_FORMAT_VMWARE,
	DUMP_FORMAT_XEN,
	DUMP_FORMAT_ETALLEN
};

struct {
	const char *name;
	int value;
} formats [] = {
	{ "default",  DUMP_FORMAT_DEFAULT },
	{ "vmware",   DUMP_FORMAT_VMWARE },
	{ "xen",      DUMP_FORMAT_XEN },
	{ "etallen",  DUMP_FORMAT_ETALLEN },
	{ NULL, 0 }
};

static int do_sanity = 0;
static int do_dump = 0;
static int dump_format = DUMP_FORMAT_DEFAULT;

int main(int argc, char **argv)
{
	const char *file = NULL;
	struct cpuid_state_t state;
	int c, ret = 0;
	int cpu_start = 0, cpu_end = 0;

	while (TRUE) {
		static struct option long_options[] = {
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{"sanity", no_argument, &do_sanity, 1},
			{"dump", no_argument, &do_dump, 1},
			{"cpu", required_argument, 0, 'c'},
			{"ignore-vendor", no_argument, &ignore_vendor, 1},
			{"parse", required_argument, 0, 'f'},
			{"format", required_argument, 0, 'o'},
			{"scan-to", required_argument, 0, 2},
			{0, 0, 0, 0}
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "c:hdvo:f:", long_options, &option_index);
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
			if (sscanf(optarg, "%d", &cpu_start) != 1) {
				printf("Option --cpu= requires an integer parameter.\n");
				exit(1);
			}
			break;
		case 'd':
			do_dump = 1;
			if (cpu_start == 0 && cpu_end == 0)
				cpu_start = -1;
			break;
		case 'f':
			file = optarg;
			break;
		case 'o':
			assert(optarg);
			for (c = 0; formats[c].name != NULL; c++) {
				if (0 == strcmp(optarg, formats[c].name)) {
					do_dump = 1;
					dump_format = formats[c].value;
					break;
				}
			}
			if (!formats[c].name) {
				printf("Unrecognized format: '%s'\n", optarg);
				exit(1);
			}
			break;
		case 'v':
			version();
		case 'h':
		case '?':
		default:
			usage(argv[0]);
		}
	}

	INIT_CPUID_STATE(&state);

	switch(dump_format) {
	case DUMP_FORMAT_DEFAULT:
		state.cpuid_print = cpuid_dump_normal;
		break;
	case DUMP_FORMAT_VMWARE:
		cpu_start = cpu_end = 0;
		state.cpuid_print = cpuid_dump_vmware;
		break;
	case DUMP_FORMAT_XEN:
		cpu_start = cpu_end = 0;
		state.cpuid_print = cpuid_dump_xen;
		printf("cpuid = [\n");
		break;
	case DUMP_FORMAT_ETALLEN:
		state.cpuid_print = cpuid_dump_etallen;
		break;
	}

	if (file) {
		cpuid_load_from_file(file, &state);
		state.cpuid_call = cpuid_stub;
		state.thread_bind = thread_bind_stub;
		state.thread_count = thread_count_stub;
	}

	if (cpu_start == -1) {
		cpu_start = 0;
		cpu_end = state.thread_count(&state) - 1;
	} else {
		cpu_end = cpu_start;
	}

	if ((uint32_t)cpu_start >= state.thread_count(&state)) {
		printf("CPU %d doesn't seem to exist.\n", cpu_start);
		exit(1);
	}

	for (c = cpu_start; c <= cpu_end; c++) {
		state.thread_bind(&state, c);

		switch(dump_format) {
		case DUMP_FORMAT_DEFAULT:
		case DUMP_FORMAT_ETALLEN:
			printf("CPU %d:\n", c);
			break;
		}
		run_cpuid(&state, do_dump);
	}

	if (do_dump && dump_format == DUMP_FORMAT_XEN) {
		/* This isn't a pretty way to do this. I suspect we might
		 * want pre and post print hooks.
		 */
		printf("]\n");
	}

	if (do_sanity) {
		ret = sanity_run(&state);
	}

	FREE_CPUID_STATE(&state);

	return ret;
}
