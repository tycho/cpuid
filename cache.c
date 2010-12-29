#include "prefix.h"

#include "cache.h"
#include "cpuid.h"
#include "feature.h"
#include "state.h"
#include "vendor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *descs[] = {
    /* 00 */ NULL,
    /* 01 */ "Code TLB: 4KB pages, 4-way associativity, 32 entries",
    /* 02 */ "Code TLB: 4MB pages, fully associative, 2 entries",
    /* 03 */ "Data TLB: 4KB pages, 4-way associativity, 64 entries",
    /* 04 */ "Data TLB: 4MB pages, 4-way associativity, 8 entries",
    /* 05 */ "Data TLB: 4MB pages, 4-way associativity, 32 entries",
    /* 06 */ "L1 code cache: 8KB, 4-way associativity, 32 bytes/line",
    /* 07 */ NULL,
    /* 08 */ "L1 code cache: 16KB, 4-way associativity, 32 bytes/line",
    /* 09 */ "L1 code cache: 32KB, 4-way associativity, 64 bytes/line",
    /* 0a */ "L1 data cache: 8KB, 2-way associativity, 32 bytes/line",
    /* 0b */ "Code TLB: 4MB pages, 4-way associativity, 4 entries (undocumented)",
    /* 0c */ "L1 data cache: 16KB, 4-way associativity, 32 bytes/line",
    /* 0d */ "L1 data cache: 16KB, 4-way associativity, 64 bytes/line, ECC",
    /* 0e */ "L1 data cache: 24KB, 6-way associativity, 64 bytes/line (undocumented)",
    /* 0f */ NULL,
    /* 10 */ "L1 data cache, 16KB, 4-way associativity, 32 bytes/line (IA-64)",
    /* 11 */ NULL,
    /* 12 */ NULL,
    /* 13 */ NULL,
    /* 14 */ NULL,
    /* 15 */ "L1 code cache, 16KB, 4-way associativity, 32 bytes/line (IA-64)",
    /* 16 */ NULL,
    /* 17 */ NULL,
    /* 18 */ NULL,
    /* 19 */ NULL,
    /* 1a */ "L2 cache, 96KB, 6-way associativity, 64 bytes/line (IA-64)",
    /* 1b */ NULL,
    /* 1c */ NULL,
    /* 1d */ NULL,
    /* 1e */ NULL,
    /* 1f */ NULL,
    /* 20 */ NULL,
    /* 21 */ "L2 cache: 256KB, 8-way associativity, 64 bytes/line",
    /* 22 */ "L3 cache: 512KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 23 */ "L3 cache: 1MB, 8-way associativity, sectored cache, 64 bytes/line",
    /* 24 */ NULL,
    /* 25 */ "L3 cache: 2MB, 8-way associativity, sectored cache, 64 bytes/line",
    /* 26 */ NULL,
    /* 27 */ NULL,
    /* 28 */ NULL,
    /* 29 */ "L3 cache: 4MB, 8-way associativity, sectored cache, 64 bytes/line",
    /* 2a */ NULL,
    /* 2b */ NULL,
    /* 2c */ "L1 data cache: 32KB, 8-way set assocative, 64 bytes/line",
    /* 2d */ NULL,
    /* 2e */ NULL,
    /* 2f */ NULL,
    /* 30 */ "L1 code cache: 32KB, 8-way associativity, 64 bytes/line",
    /* 31 */ NULL,
    /* 32 */ NULL,
    /* 33 */ NULL,
    /* 34 */ NULL,
    /* 35 */ NULL,
    /* 36 */ NULL,
    /* 37 */ NULL,
    /* 38 */ NULL,
    /* 39 */ "L2 cache: 128KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 3a */ "L2 cache: 192KB, 6-way associativity, sectored cache, 64 bytes/line",
    /* 3b */ "L2 cache: 128KB, 2-way associativity, sectored cache, 64 bytes/line",
    /* 3c */ "L2 cache: 256KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 3d */ "L2 cache: 384KB, 6-way associativity, sectored cache, 64 bytes/line",
    /* 3e */ "L2 cache: 512KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 3f */ NULL,
    /* 40 */ "No L2 cache, or if L2 cache exists, no L3 cache",
    /* 41 */ "L2 cache: 128KB, 4-way associativity, 32 bytes/line",
    /* 42 */ "L2 cache: 256KB, 4-way associativity, 32 bytes/line",
    /* 43 */ "L2 cache: 512KB, 4-way associativity, 32 bytes/line",
    /* 44 */ "L2 cache: 1MB, 4-way associativity, 32 bytes/line",
    /* 45 */ "L2 cache: 2MB, 4-way associativity, 32 bytes/line",
    /* 46 */ "L3 cache: 4MB, 4-way associativity, 64 bytes/line",
    /* 47 */ "L3 cache: 8MB, 8-way associativity, 64 bytes/line",
    /* 48 */ "L2 cache: 3MB, 12-way associativity, 64 bytes/line",
    /* 49 */ NULL, /* Documented, but a special case. */
    /* 4a */ "L3 cache: 6MB, 12-way associativity, 64 bytes/line",
    /* 4b */ "L3 cache: 8MB, 16-way associativity, 64 bytes/line",
    /* 4c */ "L3 cache: 12MB, 12-way associativity, 64 bytes/line",
    /* 4d */ "L3 cache: 16MB, 16-way associativity, 64 bytes/line",
    /* 4e */ "L2 cache: 6MB, 24-way associativity, 64 bytes/line",
    /* 4f */ "Code TLB: 4KB pages, unknown associativity, 32 entries (undocumented)",
    /* 50 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 64 entries",
    /* 51 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 128 entries",
    /* 52 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 256 entries",
    /* 53 */ NULL,
    /* 54 */ NULL,
    /* 55 */ "Code TLB: 2MB or 4MB pages, fully associative, 256 entries",
    /* 56 */ "L1 Data TLB: 4MB pages, 4-way associativity associative, 16 entries",
    /* 57 */ "L1 Data TLB: 4KB pages, 4-way associativity, 16 entries",
    /* 58 */ NULL,
    /* 59 */ "L0 data TLB: 4KB pages, fully associative, 16 entries (undocumented)",
    /* 5a */ "Data TLB0: 2MB or 4MB pages, 4-way associativity, 32 entries",
    /* 5b */ "Data TLB: 4KB or 4MB pages, fully associative, 64 entries",
    /* 5c */ "Data TLB: 4KB or 4MB pages, fully associative, 128 entries",
    /* 5d */ "Data TLB: 4KB or 4MB pages, fully associative, 256 entries",
    /* 5e */ NULL,
    /* 5f */ NULL,
    /* 60 */ "L1 data cache: 16KB, 8-way associativity, sectored cache, 64 bytes/line",
    /* 61 */ NULL,
    /* 62 */ NULL,
    /* 63 */ NULL,
    /* 64 */ NULL,
    /* 65 */ NULL,
    /* 66 */ "L1 data cache: 8KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 67 */ "L1 data cache: 16KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 68 */ "L1 data cache: 32KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 69 */ NULL,
    /* 6a */ NULL,
    /* 6b */ NULL,
    /* 6c */ NULL,
    /* 6d */ NULL,
    /* 6e */ NULL,
    /* 6f */ NULL,
    /* 70 */ "12K-uops, 8-way associativity",
    /* 71 */ "16K-uops, 8-way associativity",
    /* 72 */ "32K-uops, 8-way associativity",
    /* 73 */ "64K-uops, 8-way associativity",
    /* 74 */ NULL,
    /* 75 */ NULL,
    /* 76 */ NULL,
    /* 77 */ "L1 code cache: 16KB, 4-way associativity, 64 bytes/line, sectored (IA-64)",
    /* 78 */ "L2 cache: 1MB, 4-way associativity, 64 bytes/line",
    /* 79 */ "L2 cache: 128KB, 8-way associativity, sectored cache, 64 bytes/line",
    /* 7a */ "L2 cache: 256KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 7b */ "L2 cache: 512KB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 7c */ "L2 cache: 1MB, 4-way associativity, sectored cache, 64 bytes/line",
    /* 7d */ "L2 cache: 2MB, 8-way associativity, 64 bytes/line",
    /* 7e */ "L2 cache: 256KB, 8-way associativity, 128 bytes/line, sectored (IA-64)",
    /* 7f */ "L2 cache: 512KB, 2-way associativity, 64 bytes/line",
    /* 80 */ "L2 cache: 512KB, 8-way associativity, 64 bytes/line (undocumented)",
    /* 81 */ "L2 cache: 128KB, 8-way associativity, 32 bytes/line (undocumented)",
    /* 82 */ "L2 cache: 256KB, 8-way associativity, 32 bytes/line",
    /* 83 */ "L2 cache: 512KB, 8-way associativity, 32 bytes/line",
    /* 84 */ "L2 cache: 1MB, 8-way associativity, 32 bytes/line",
    /* 85 */ "L2 cache: 2MB, 8-way associativity, 32 bytes/line",
    /* 86 */ "L2 cache: 512KB, 4-way associativity, 64 bytes/line",
    /* 87 */ "L2 cache: 1MB, 8-way associativity, 64 bytes/line",
    /* 88 */ "L3 cache: 2MB, 4-way associativity, 64 bytes/line (IA-64)",
    /* 89 */ "L3 cache: 4MB, 4-way associativity, 64 bytes/line (IA-64)",
    /* 8a */ "L3 cache: 8MB, 4-way associativity, 64 bytes/line (IA-64)",
    /* 8b */ NULL,
    /* 8c */ NULL,
    /* 8d */ "L3 cache: 3MB, 12-way associativity, 128 bytes/line (IA-64)",
    /* 8e */ NULL,
    /* 8f */ NULL,
    /* 90 */ "Code TLB: 4KB-256MB pages, fully associative, 64 entries (IA-64)",
    /* 91 */ NULL,
    /* 92 */ NULL,
    /* 93 */ NULL,
    /* 94 */ NULL,
    /* 95 */ NULL,
    /* 96 */ "L1 data TLB, 4KB-256MB pages, fully associative, 32 entries (IA-64)",
    /* 97 */ NULL,
    /* 98 */ NULL,
    /* 99 */ NULL,
    /* 9a */ NULL,
    /* 9b */ "L2 data TLB, 4KB-256MB pages, fully associative, 96 entries (IA-64)",
    /* 9c */ NULL,
    /* 9d */ NULL,
    /* 9e */ NULL,
    /* 9f */ NULL,
    /* a0 */ NULL,
    /* a1 */ NULL,
    /* a2 */ NULL,
    /* a3 */ NULL,
    /* a4 */ NULL,
    /* a5 */ NULL,
    /* a6 */ NULL,
    /* a7 */ NULL,
    /* a8 */ NULL,
    /* a9 */ NULL,
    /* aa */ NULL,
    /* ab */ NULL,
    /* ac */ NULL,
    /* ad */ NULL,
    /* ae */ NULL,
    /* af */ NULL,
    /* b0 */ "Code TLB: 4KB pages, 4-way associativity, 128 entries",
    /* b1 */ "Code TLB: 2MB pages, 4-way associativity, 8 entries",
    /* b2 */ "Data TLB: 4KB pages, 4-way associativity, 64 entries",
    /* b3 */ "Data TLB: 4KB pages, 4-way associativity, 128 entries",
    /* b4 */ "Data TLB: 4KB pages, 4-way associativity, 256 entries",
    /* b5 */ NULL,
    /* b6 */ NULL,
    /* b7 */ NULL,
    /* b8 */ NULL,
    /* b9 */ NULL,
    /* ba */ "Data TLB: 4KB pages, 4-way associativity, 64 entries (undocumented)",
    /* bb */ NULL,
    /* bc */ NULL,
    /* bd */ NULL,
    /* be */ NULL,
    /* bf */ NULL,
    /* c0 */ "Data TLB: 4KB and 4MB pages, 4-way associativity, 8 entries (undocumented)",
    /* c1 */ NULL,
    /* c2 */ NULL,
    /* c3 */ NULL,
    /* c4 */ NULL,
    /* c5 */ NULL,
    /* c6 */ NULL,
    /* c7 */ NULL,
    /* c8 */ NULL,
    /* c9 */ NULL,
    /* ca */ "Shared L2 TLB: 4KB pages, 4-way associativity, 512 entries",
    /* cb */ NULL,
    /* cc */ NULL,
    /* cd */ NULL,
    /* ce */ NULL,
    /* cf */ NULL,
    /* d0 */ "L3 cache: 512KB, 4-way associativity, 64 bytes/line",
    /* d1 */ "L3 cache: 1MB, 4-way associativity, 64 bytes/line",
    /* d2 */ "L3 cache: 2MB, 4-way associativity, 64 bytes/line",
    /* d3 */ NULL,
    /* d4 */ NULL,
    /* d5 */ NULL,
    /* d6 */ "L3 cache: 1MB, 8-way associativity, 64 bytes/line",
    /* d7 */ "L3 cache: 2MB, 8-way associativity, 64 bytes/line",
    /* d8 */ "L3 cache: 4MB, 8-way associativity, 64 bytes/line",
    /* d9 */ NULL,
    /* da */ NULL,
    /* db */ NULL,
    /* dc */ "L3 cache: 1.5MB, 12-way associativity, 64 bytes/line",
    /* dd */ "L3 cache: 3MB, 12-way associativity, 64 bytes/line",
    /* de */ "L3 cache: 6MB, 12-way associativity, 64 bytes/line",
    /* df */ NULL,
    /* e0 */ NULL,
    /* e1 */ NULL,
    /* e2 */ "L3 cache: 2MB, 16-way associativity, 64 bytes/line",
    /* e3 */ "L3 cache: 4MB, 16-way associativity, 64 bytes/line",
    /* e4 */ "L3 cache: 8MB, 16-way associativity, 64 bytes/line",
    /* e5 */ NULL,
    /* e6 */ NULL,
    /* e7 */ NULL,
    /* e8 */ NULL,
    /* e9 */ NULL,
    /* ea */ "L3 cache: 12MB, 24-way associativity, 64 bytes/line",
    /* eb */ "L3 cache: 18MB, 24-way associativity, 64 bytes/line",
    /* ec */ "L3 cache: 24MB, 24-way associativity, 64 bytes/line",
    /* ed */ NULL,
    /* ee */ NULL,
    /* ef */ NULL,
    /* f0 */ "64 byte prefetching",
    /* f1 */ "128 byte prefetching",
    /* f2 */ NULL,
    /* f3 */ NULL,
    /* f4 */ NULL,
    /* f5 */ NULL,
    /* f6 */ NULL,
    /* f7 */ NULL,
    /* f8 */ NULL,
    /* f9 */ NULL,
    /* fa */ NULL,
    /* fb */ NULL,
    /* fc */ NULL,
    /* fd */ NULL,
    /* fe */ NULL,
    /* ff */ NULL
};

static const char *descriptor_49[] = {
	/* 00 */ "L2 cache: 4MB, 16-way associativity, 64 bytes/line",
	/* 01 */ "L3 cache: 4MB, 16-way associativity, 64 bytes/line",
};

int entry_comparator(const void *a, const void *b)
{
	/* Make 'prefetching' lines show last. */
	if (strstr(*(const char **)a, "prefetch")) return 1;
	if (strstr(*(const char **)b, "prefetch")) return -1;

	/* Simple string compare. */
	return strcmp(*(const char **)a, *(const char **)b);
}

void print_intel_caches(struct cpu_regs_t *regs, const struct cpu_signature_t *sig)
{
	uint8_t buf[16], i;

	/* It's only possible to have 16 entries on a single line, but
	   we need a 17th for a sentinel value of zero. */
	const char *entries[17];

	const char **eptr = entries;

	/* Only zero last element. All other entries are initialized below. */
	entries[16] = 0;

	memset(buf, 0, sizeof(buf));

	*(uint32_t *)&buf[0x0] = regs->eax >> 8;
	if ((regs->ebx & (1 << 31)) == 0)
		*(uint32_t *)&buf[0x3] = regs->ebx;
	if ((regs->ecx & (1 << 31)) == 0)
		*(uint32_t *)&buf[0x7] = regs->ecx;
	if ((regs->edx & (1 << 31)) == 0)
		*(uint32_t *)&buf[0xB] = regs->edx;

	for (i = 0; i < 0xF; i++) {
		const char *desc = descs[buf[i]];

		/* Fetch a description. */
		if (desc) {
			*eptr++ = desc;
		} else {
			if (buf[i] == 0x49) {
				/* A very stupid special case. AP-485 says this
				 * is a L3 cache for Intel Xeon processor MP,
				 * Family 0Fh, Model 06h, while it's a L2 cache
				 * on everything else.
				 */
				*eptr++ = (sig->family == 0x0F && sig->model == 0x06) ?
				          descriptor_49[1] : descriptor_49[0];
			}
			else if (buf[i] != 0x00)
			{
				/* This one we can just print right away. Its exact string
				   will vary, and we wouldn't know how to sort it anyway. */
				printf("  Unknown cache descriptor (0x%02x)\n", buf[i]);
			}
		}
	}

	i = eptr - entries;

	for(; eptr < &entries[17]; eptr++) {
		*eptr = 0;
	}

	/* Sort alphabetically. Makes it a heck of a lot easier to read. */
	qsort(entries, i, sizeof(const char *), entry_comparator);

	/* Print the entries. */
	eptr = entries;
	while (*eptr) {
		printf("  %s\n", *eptr);
		eptr++;
	}
}

