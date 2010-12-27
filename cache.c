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
    /* 01 */ "Code TLB: 4KB pages, 4-way set associative, 32 entries",
    /* 02 */ "Code TLB: 4MB pages, fully associative, 2 entries",
    /* 03 */ "Data TLB: 4KB pages, 4-way set associative, 64 entries",
    /* 04 */ "Data TLB: 4MB pages, 4-way set associative, 8 entries",
    /* 05 */ "Data TLB: 4MB pages, 4-way set associative, 32 entries",
    /* 06 */ "1st-level code cache: 8KB, 4-way set associative, 32 byte line size",
    /* 07 */ NULL,
    /* 08 */ "1st-level code cache: 16KB, 4-way set associative, 32 byte line size",
    /* 09 */ "1st-level code cache: 32KB, 4-way set associative, 64 byte line size",
    /* 0a */ "1st-level data cache: 8KB, 2-way set associative, 32 byte line size",
    /* 0b */ NULL,
    /* 0c */ "1st-level data cache: 16KB, 4-way set associative, 32 byte line size",
    /* 0d */ "1st-level data cache: 16KB, 4-way set associative, 64 byte line size, ECC",
    /* 0e */ NULL,
    /* 0f */ NULL,
    /* 10 */ NULL,
    /* 11 */ NULL,
    /* 12 */ NULL,
    /* 13 */ NULL,
    /* 14 */ NULL,
    /* 15 */ NULL,
    /* 16 */ NULL,
    /* 17 */ NULL,
    /* 18 */ NULL,
    /* 19 */ NULL,
    /* 1a */ NULL,
    /* 1b */ NULL,
    /* 1c */ NULL,
    /* 1d */ NULL,
    /* 1e */ NULL,
    /* 1f */ NULL,
    /* 20 */ NULL,
    /* 21 */ "2nd-level cache: 256KB, 8-way set associative, 64 byte line size",
    /* 22 */ "3rd-level cache: 512KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 23 */ "3rd-level cache: 1MB, 8-way set associative, sectored cache, 64 byte line size",
    /* 24 */ NULL,
    /* 25 */ "3rd-level cache: 2MB, 8-way set associative, sectored cache, 64 byte line size",
    /* 26 */ NULL,
    /* 27 */ NULL,
    /* 28 */ NULL,
    /* 29 */ "3rd-level cache: 4MB, 8-way set associative, sectored cache, 64 byte line size",
    /* 2a */ NULL,
    /* 2b */ NULL,
    /* 2c */ "1st-level data cache: 32KB, 8-way set assocative, 64 byte line size",
    /* 2d */ NULL,
    /* 2e */ NULL,
    /* 2f */ NULL,
    /* 30 */ "1st-level code cache: 32KB, 8-way set associative, 64 byte line size",
    /* 31 */ NULL,
    /* 32 */ NULL,
    /* 33 */ NULL,
    /* 34 */ NULL,
    /* 35 */ NULL,
    /* 36 */ NULL,
    /* 37 */ NULL,
    /* 38 */ NULL,
    /* 39 */ "2nd-level cache: 128KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 3a */ "2nd-level cache: 192KB, 6-way set associative, sectored cache, 64 byte line size",
    /* 3b */ "2nd-level cache: 128KB, 2-way set associative, sectored cache, 64 byte line size",
    /* 3c */ "2nd-level cache: 256KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 3d */ "2nd-level cache: 384KB, 6-way set associative, sectored cache, 64 byte line size",
    /* 3e */ "2nd-level cache: 512KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 3f */ NULL,
    /* 40 */ "No 2nd-level cache, or if 2nd-level cache exists, no 3rd-level cache",
    /* 41 */ "2nd-level cache: 128KB, 4-way set associative, 32 byte line size",
    /* 42 */ "2nd-level cache: 256KB, 4-way set associative, 32 byte line size",
    /* 43 */ "2nd-level cache: 512KB, 4-way set associative, 32 byte line size",
    /* 44 */ "2nd-level cache: 1MB, 4-way set associative, 32 byte line size",
    /* 45 */ "2nd-level cache: 2MB, 4-way set associative, 32 byte line size",
    /* 46 */ "3rd-level cache: 4MB, 4-way set associative, 64 byte line size",
    /* 47 */ "3rd-level cache: 8MB, 8-way set associative, 64 byte line size",
    /* 48 */ "2nd-level cache: 3MB, 12-way set associative, 64 byte line size",
    /* 49 */ NULL, /* Documented, but a special case. */
    /* 4a */ "3rd-level cache: 6MB, 12-way set associative, 64 byte line size",
    /* 4b */ "3rd-level cache: 8MB, 16-way set associative, 64 byte line size",
    /* 4c */ "3rd-level cache: 12MB, 12-way set associative, 64 byte line size",
    /* 4d */ "3rd-level cache: 16MB, 16-way set associative, 64 byte line size",
    /* 4e */ "2nd-level cache: 6MB, 24-way set associative, 64 byte line size",
    /* 4f */ NULL,
    /* 50 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 64 entries",
    /* 51 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 128 entries",
    /* 52 */ "Code TLB: 4KB, 2MB, or 4MB pages, fully associative, 256 entries",
    /* 53 */ NULL,
    /* 54 */ NULL,
    /* 55 */ "Code TLB: 2MB or 4MB pages, fully associative, 256 entries",
    /* 56 */ "L1 Data TLB: 4MB pages, 4-way set associative associative, 16 entries",
    /* 57 */ "L1 Data TLB: 4KB pages, 4-way set associative, 16 entries",
    /* 58 */ NULL,
    /* 59 */ NULL,
    /* 5a */ "Data TLB0: 2MB or 4MB pages, 4-way set associative, 32 entries",
    /* 5b */ "Data TLB: 4KB or 4MB pages, fully associative, 64 entries",
    /* 5c */ "Data TLB: 4KB or 4MB pages, fully associative, 128 entries",
    /* 5d */ "Data TLB: 4KB or 4MB pages, fully associative, 256 entries",
    /* 5e */ NULL,
    /* 5f */ NULL,
    /* 60 */ "1st-level data cache: 16KB, 8-way set associative, sectored cache, 64 byte line size",
    /* 61 */ NULL,
    /* 62 */ NULL,
    /* 63 */ NULL,
    /* 64 */ NULL,
    /* 65 */ NULL,
    /* 66 */ "1st-level data cache: 8KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 67 */ "1st-level data cache: 16KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 68 */ "1st-level data cache: 32KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 69 */ NULL,
    /* 6a */ NULL,
    /* 6b */ NULL,
    /* 6c */ NULL,
    /* 6d */ NULL,
    /* 6e */ NULL,
    /* 6f */ NULL,
    /* 70 */ "12K-uops, 8-way set associative",
    /* 71 */ "16K-uops, 8-way set associative",
    /* 72 */ "32K-uops, 8-way set associative",
    /* 73 */ "64K-uops, 8-way set associative",
    /* 74 */ NULL,
    /* 75 */ NULL,
    /* 76 */ NULL,
    /* 77 */ NULL,
    /* 78 */ "2nd-level cache: 1MB, 4-way set associative, 64 byte line size",
    /* 79 */ "2nd-level cache: 128KB, 8-way set associative, sectored cache, 64 byte line size",
    /* 7a */ "2nd-level cache: 256KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 7b */ "2nd-level cache: 512KB, 4-way set associative, sectored cache, 64 byte line size",
    /* 7c */ "2nd-level cache: 1MB, 4-way set associative, sectored cache, 64 byte line size",
    /* 7d */ "2nd-level cache: 2MB, 8-way set associative, 64 byte line size",
    /* 7e */ NULL,
    /* 7f */ "2nd-level cache: 512KB, 2-way set associative, 64 byte line size",
    /* 80 */ NULL,
    /* 81 */ NULL,
    /* 82 */ "2nd-level cache: 256KB, 8-way set associative, 32 byte line size",
    /* 83 */ "2nd-level cache: 512KB, 8-way set associative, 32 byte line size",
    /* 84 */ "2nd-level cache: 1MB, 8-way set associative, 32 byte line size",
    /* 85 */ "2nd-level cache: 2MB, 8-way set associative, 32 byte line size",
    /* 86 */ "2nd-level cache: 512KB, 4-way set associative, 64 byte line size",
    /* 87 */ "2nd-level cache: 1MB, 8-way set associative, 64 byte line size",
    /* 88 */ NULL,
    /* 89 */ NULL,
    /* 8a */ NULL,
    /* 8b */ NULL,
    /* 8c */ NULL,
    /* 8d */ NULL,
    /* 8e */ NULL,
    /* 8f */ NULL,
    /* 90 */ NULL,
    /* 91 */ NULL,
    /* 92 */ NULL,
    /* 93 */ NULL,
    /* 94 */ NULL,
    /* 95 */ NULL,
    /* 96 */ NULL,
    /* 97 */ NULL,
    /* 98 */ NULL,
    /* 99 */ NULL,
    /* 9a */ NULL,
    /* 9b */ NULL,
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
    /* b0 */ "Code TLB: 4KB pages, 4-way set associative, 128 entries",
    /* b1 */ "Code TLB: 2MB pages, 4-way set associative, 8 entries",
    /* b2 */ "Data TLB: 4KB pages, 4-way set associative, 64 entries",
    /* b3 */ "Data TLB: 4KB pages, 4-way set associative, 128 entries",
    /* b4 */ "Data TLB: 4KB pages, 4-way set associative, 256 entries",
    /* b5 */ NULL,
    /* b6 */ NULL,
    /* b7 */ NULL,
    /* b8 */ NULL,
    /* b9 */ NULL,
    /* ba */ NULL,
    /* bb */ NULL,
    /* bc */ NULL,
    /* bd */ NULL,
    /* be */ NULL,
    /* bf */ NULL,
    /* c0 */ NULL,
    /* c1 */ NULL,
    /* c2 */ NULL,
    /* c3 */ NULL,
    /* c4 */ NULL,
    /* c5 */ NULL,
    /* c6 */ NULL,
    /* c7 */ NULL,
    /* c8 */ NULL,
    /* c9 */ NULL,
    /* ca */ "Shared 2nd-level TLB: 4KB pages, 4-way set associative, 512 entries",
    /* cb */ NULL,
    /* cc */ NULL,
    /* cd */ NULL,
    /* ce */ NULL,
    /* cf */ NULL,
    /* d0 */ "3rd-level cache: 512KB, 4-way set associative, 64 byte line size",
    /* d1 */ "3rd-level cache: 1MB, 4-way set associative, 64 byte line size",
    /* d2 */ "3rd-level cache: 2MB, 4-way set associative, 64 byte line size",
    /* d3 */ NULL,
    /* d4 */ NULL,
    /* d5 */ NULL,
    /* d6 */ "3rd-level cache: 1MB, 8-way set associative, 64 byte line size",
    /* d7 */ "3rd-level cache: 2MB, 8-way set associative, 64 byte line size",
    /* d8 */ "3rd-level cache: 4MB, 8-way set associative, 64 byte line size",
    /* d9 */ NULL,
    /* da */ NULL,
    /* db */ NULL,
    /* dc */ "3rd-level cache: 1.5MB, 12-way set associative, 64 byte line size",
    /* dd */ "3rd-level cache: 3MB, 12-way set associative, 64 byte line size",
    /* de */ "3rd-level cache: 6MB, 12-way set associative, 64 byte line size",
    /* df */ NULL,
    /* e0 */ NULL,
    /* e1 */ NULL,
    /* e2 */ "3rd-level cache: 2MB, 16-way set associative, 64 byte line size",
    /* e3 */ "3rd-level cache: 4MB, 16-way set associative, 64 byte line size",
    /* e4 */ "3rd-level cache: 8MB, 16-way set associative, 64 byte line size",
    /* e5 */ NULL,
    /* e6 */ NULL,
    /* e7 */ NULL,
    /* e8 */ NULL,
    /* e9 */ NULL,
    /* ea */ "3rd-level cache: 12MB, 24-way set associative, 64 byte line size",
    /* eb */ "3rd-level cache: 18MB, 24-way set associative, 64 byte line size",
    /* ec */ "3rd-level cache: 24MB, 24-way set associative, 64 byte line size",
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
	/* 00 */ "2nd-level cache: 4MB, 16-way set associative, 64 byte line size",
	/* 01 */ "3rd-level cache: 4MB, 16-way set associative, 64 byte line size",
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

	*(uint32_t *)&buf[0x0] = regs->eax >> 8;
	*(uint32_t *)&buf[0x3] = regs->ebx;
	*(uint32_t *)&buf[0x7] = regs->ecx;
	*(uint32_t *)&buf[0xB] = regs->edx;

	for (i = 0; i < 0xF; i++) {
		const char *desc = descs[buf[i]];

		/* Fetch a description. */
		if (desc) {
			*eptr++ = desc;
		} else {
			if (buf[i] == 0x49) {
				/* A very stupid special case. AP-485 says this
				 * is a 3rd-level cache for Intel Xeon processor MP,
				 * Family 0Fh, Model 06h, while it's a 2nd-level cache
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

