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
uint32_t scan_to = 0;

const uint32_t ranges[] = {
	0x00000000,
	0x40000000,
	0x80000000,
	0x80860000,
	0xC0000000,
	0xFFFFFFFF
};

static void run_cpuid(struct cpuid_state_t *state, int dump)
{
	uint32_t i;
	const uint32_t *r;
	struct cpu_regs_t cr_tmp;
	const struct cpuid_leaf_handler_index_t *h;

	for (r = ranges; *r != 0xFFFFFFFF; r++) {
		state->curmax = *r;
		for (i = *r; i <= (scan_to ? *r + scan_to : state->curmax); i++) {

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
	DUMP_FORMAT_DEFAULT,
	DUMP_FORMAT_VMWARE,
	DUMP_FORMAT_XEN,
	DUMP_FORMAT_ETALLEN
};

static int do_sanity = 0;
static int do_dump = 0;
static int dump_format = DUMP_FORMAT_DEFAULT;
static int cpu_index = 0;

int main(int argc, char **argv)
{
	const char *file = NULL;
	struct cpuid_state_t state;
	int c, ret = 0;

	while (TRUE) {
		static struct option long_options[] = {
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{"sanity", no_argument, &do_sanity, 1},
			{"dump", no_argument, &do_dump, 1},
			{"cpu", required_argument, 0, 'c'},
			{"ignore-vendor", no_argument, &ignore_vendor, 1},
			{"parse", required_argument, 0, 'f'},
			{"vmware-vmx", no_argument, &dump_format, DUMP_FORMAT_VMWARE},
			{"etallen", no_argument, &dump_format, DUMP_FORMAT_ETALLEN},
			{"xen", no_argument, &dump_format, DUMP_FORMAT_XEN},
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

	thread_bind(cpu_index);

	INIT_CPUID_STATE(&state);

	/* Non-default dump format settings imply --dump. */
	switch(dump_format) {
	case DUMP_FORMAT_DEFAULT:
		state.cpuid_print = cpuid_dump_normal;
		break;
	case DUMP_FORMAT_VMWARE:
		do_dump = 1;
		state.cpuid_print = cpuid_dump_vmware;
		break;
	case DUMP_FORMAT_XEN:
		do_dump = 1;
		state.cpuid_print = cpuid_dump_xen;
		printf("cpuid = [\n");
		break;
	case DUMP_FORMAT_ETALLEN:
		do_dump = 1;
		state.cpuid_print = cpuid_dump_etallen;
		break;
	}

	if (file) {
		cpuid_load_from_file(file, &state);
		state.cpuid_call = cpuid_pseudo;
	}

	run_cpuid(&state, do_dump);

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
