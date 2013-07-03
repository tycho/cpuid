/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2013, Steven Noonan <steven@uplinklabs.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "prefix.h"

#include "cache.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	DATA_TLB,
	CODE_TLB,
	SHARED_TLB,
	DATA,
	CODE,
	TRACE,
	UNIFIED
} cache_type_t;

typedef enum {
	NO,
	L0,
	L1,
	L2,
	L3
} cache_level_t;

typedef enum {
	NONE = 0x0,
	UNDOCUMENTED = 0x1,
	IA64 = 0x2,
	ECC = 0x4,
	SECTORED = 0x8,
	PAGES_4K = 0x10,
	PAGES_2M = 0x20,
	PAGES_4M = 0x40
} extra_attrs_t;

struct cache_desc_t {
	cache_level_t level;
	cache_type_t type;
	uint32_t size;
	uint32_t attrs;
	uint8_t assoc;
	uint8_t linesize;
};

struct cache_desc_index_t {
	uint8_t descriptor;
	struct cache_desc_t desc;
};

#define MB * 1024
static const struct cache_desc_index_t descs[] = {
	{ 0x01, {NO, CODE_TLB,    32, PAGES_4K, 0x04, 0}},
	{ 0x02, {NO, CODE_TLB,     2, PAGES_4M, 0xFF, 0}},
	{ 0x03, {NO, DATA_TLB,    64, PAGES_4K, 0x04, 0}},
	{ 0x04, {NO, DATA_TLB,     8, PAGES_4M, 0x04, 0}},
	{ 0x05, {NO, DATA_TLB,    32, PAGES_4M, 0x04, 0}},
	{ 0x06, {L1, CODE,         8, NONE, 0x04, 32}},
	{ 0x08, {L1, CODE,        16, NONE, 0x04, 32}},
	{ 0x09, {L1, CODE,        32, NONE, 0x04, 64}},
	{ 0x0a, {L1, DATA,         8, NONE, 0x02, 32}},
	{ 0x0b, {L1, CODE_TLB,     4, PAGES_4M, 0x04, 0}},
	{ 0x0c, {L1, DATA,        16, NONE, 0x04, 32}},
	{ 0x0d, {L1, DATA,        16, ECC, 0x04, 64}},
	{ 0x0e, {L1, DATA,        24, NONE, 0x06, 64}},
	{ 0x10, {L1, DATA,        16, IA64, 0x04, 32}},
	{ 0x15, {L1, CODE,        16, IA64, 0x04, 32}},
	{ 0x1a, {L2, UNIFIED,     96, IA64, 0x06, 64}},
	{ 0x21, {L2, UNIFIED,    256, NONE, 0x08, 64}},
	{ 0x22, {L3, UNIFIED,    512, SECTORED, 0x04, 64}},
	{ 0x23, {L3, UNIFIED,   1 MB, SECTORED, 0x08, 64}},
	{ 0x25, {L3, UNIFIED,   2 MB, SECTORED, 0x08, 64}},
	{ 0x29, {L3, UNIFIED,   4 MB, SECTORED, 0x08, 64}},
	{ 0x2c, {L1, DATA,        32, NONE, 0x08, 64}},
	{ 0x30, {L1, CODE,        32, NONE, 0x08, 64}},
	{ 0x39, {L2, UNIFIED,    128, SECTORED, 0x04, 64}},
	{ 0x3a, {L2, UNIFIED,    192, SECTORED, 0x06, 64}},
	{ 0x3b, {L2, UNIFIED,    128, SECTORED, 0x02, 64}},
	{ 0x3c, {L2, UNIFIED,    256, SECTORED, 0x04, 64}},
	{ 0x3d, {L2, UNIFIED,    384, SECTORED, 0x06, 64}},
	{ 0x3e, {L2, UNIFIED,    512, SECTORED, 0x04, 64}},
	{ 0x41, {L2, UNIFIED,    128, NONE, 0x04, 32}},
	{ 0x42, {L2, UNIFIED,    256, NONE, 0x04, 32}},
	{ 0x43, {L2, UNIFIED,    512, NONE, 0x04, 32}},
	{ 0x44, {L2, UNIFIED,   1 MB, NONE, 0x04, 32}},
	{ 0x45, {L2, UNIFIED,   2 MB, NONE, 0x04, 32}},
	{ 0x46, {L3, UNIFIED,   4 MB, NONE, 0x04, 64}},
	{ 0x47, {L3, UNIFIED,   8 MB, NONE, 0x08, 64}},
	{ 0x48, {L2, UNIFIED,   3 MB, NONE, 0x0C, 64}},
	{ 0x4a, {L3, UNIFIED,   6 MB, NONE, 0x0C, 64}},
	{ 0x4b, {L3, UNIFIED,   8 MB, NONE, 0x10, 64}},
	{ 0x4c, {L3, UNIFIED,  12 MB, NONE, 0x0C, 64}},
	{ 0x4d, {L3, UNIFIED,  16 MB, NONE, 0x10, 64}},
	{ 0x4e, {L2, UNIFIED,   6 MB, NONE, 0x18, 64}},
	{ 0x4f, {NO, CODE_TLB,    32, PAGES_4K, 0x00, 0}},
	{ 0x50, {NO, CODE_TLB,    64, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0}},
	{ 0x51, {NO, CODE_TLB,   128, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0}},
	{ 0x52, {NO, CODE_TLB,   256, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0}},
	{ 0x55, {NO, CODE_TLB,   256, PAGES_2M | PAGES_4M, 0xFF, 0}},
	{ 0x56, {L0, DATA_TLB,    16, PAGES_4M, 0x04, 0}},
	{ 0x57, {L0, DATA_TLB,    16, PAGES_4K, 0x04, 0}},
	{ 0x59, {L0, DATA_TLB,    16, PAGES_4K, 0xFF, 0}},
	{ 0x5a, {NO, DATA_TLB,    32, PAGES_2M | PAGES_4M, 0x04, 0}},
	{ 0x5b, {NO, DATA_TLB,    64, PAGES_4K | PAGES_4M, 0xFF, 0}},
	{ 0x5c, {NO, DATA_TLB,   128, PAGES_4K | PAGES_4M, 0xFF, 0}},
	{ 0x5d, {NO, DATA_TLB,   256, PAGES_4K | PAGES_4M, 0xFF, 0}},
	{ 0x60, {L1, DATA,        16, SECTORED, 0x08, 64}},
	{ 0x66, {L1, DATA,         8, SECTORED, 0x04, 64}},
	{ 0x67, {L1, DATA,        16, SECTORED, 0x04, 64}},
	{ 0x68, {L1, DATA,        32, SECTORED, 0x04, 64}},
	{ 0x70, {L1, TRACE,       12, NONE, 0x08, 0}},
	{ 0x71, {L1, TRACE,       16, NONE, 0x08, 0}},
	{ 0x72, {L1, TRACE,       32, NONE, 0x08, 0}},
	{ 0x73, {L1, TRACE,       64, UNDOCUMENTED, 0x08, 0}},
	{ 0x76, {NO, CODE_TLB,     8, PAGES_2M | PAGES_4M, 0xFF, 0}},
	{ 0x77, {L1, CODE,        16, SECTORED | IA64, 0x04, 64}},
	{ 0x78, {L2, UNIFIED,   1 MB, NONE, 0x04, 64}},
	{ 0x79, {L2, UNIFIED,    128, SECTORED, 0x08, 64}},
	{ 0x7a, {L2, UNIFIED,    256, SECTORED, 0x04, 64}},
	{ 0x7b, {L2, UNIFIED,    512, SECTORED, 0x04, 64}},
	{ 0x7c, {L2, UNIFIED,   1 MB, SECTORED, 0x04, 64}},
	{ 0x7d, {L2, UNIFIED,   2 MB, NONE, 0x08, 64}},
	{ 0x7e, {L2, UNIFIED,    256, SECTORED | IA64, 0x08, 128}},
	{ 0x7f, {L2, UNIFIED,    512, NONE, 0x02, 64}},
	{ 0x80, {L2, UNIFIED,    512, NONE, 0x08, 64}},
	{ 0x81, {L2, UNIFIED,    128, UNDOCUMENTED, 0x08, 32}},
	{ 0x82, {L2, UNIFIED,    256, NONE, 0x08, 32}},
	{ 0x83, {L2, UNIFIED,    512, NONE, 0x08, 32}},
	{ 0x84, {L2, UNIFIED,   1 MB, NONE, 0x08, 32}},
	{ 0x85, {L2, UNIFIED,   2 MB, NONE, 0x08, 32}},
	{ 0x86, {L2, UNIFIED,    512, NONE, 0x04, 64}},
	{ 0x87, {L2, UNIFIED,   1 MB, NONE, 0x08, 64}},
	{ 0x88, {L3, UNIFIED,   2 MB, IA64, 0x04, 64}},
	{ 0x89, {L3, UNIFIED,   4 MB, IA64, 0x04, 64}},
	{ 0x8a, {L3, UNIFIED,   8 MB, IA64, 0x04, 64}},
	{ 0x8d, {L3, UNIFIED,   3 MB, IA64, 0x0C, 128}},
	{ 0xb0, {NO, CODE_TLB,   128, PAGES_4K, 0x04, 0}},
	{ 0xb1, {NO, CODE_TLB,     8, PAGES_2M, 0x04, 0}},
	{ 0xb2, {NO, DATA_TLB,    64, PAGES_4K, 0x04, 0}},
	{ 0xb3, {NO, DATA_TLB,   128, PAGES_4K, 0x04, 0}},
	{ 0xb4, {L1, DATA_TLB,   256, PAGES_4K, 0x04, 0}},
	{ 0xba, {L1, DATA_TLB,    64, PAGES_4K, 0x04, 0}},
	{ 0xc0, {NO, DATA_TLB,     8, PAGES_4K | PAGES_4M, 0x04, 0}},
	{ 0xca, {L2, SHARED_TLB, 512, PAGES_4K, 0x04, 0}},
	{ 0xd0, {L3, UNIFIED,    512, NONE, 0x04, 64}},
	{ 0xd1, {L3, UNIFIED,   1 MB, NONE, 0x04, 64}},
	{ 0xd2, {L3, UNIFIED,   2 MB, NONE, 0x04, 64}},
	{ 0xd6, {L3, UNIFIED,   1 MB, NONE, 0x08, 64}},
	{ 0xd7, {L3, UNIFIED,   2 MB, NONE, 0x08, 64}},
	{ 0xd8, {L3, UNIFIED,   4 MB, NONE, 0x08, 64}},
	{ 0xdc, {L3, UNIFIED, 1.5 MB, NONE, 0x0C, 64}},
	{ 0xdd, {L3, UNIFIED,   3 MB, NONE, 0x0C, 64}},
	{ 0xde, {L3, UNIFIED,   6 MB, NONE, 0x0C, 64}},
	{ 0xe2, {L3, UNIFIED,   2 MB, NONE, 0x10, 64}},
	{ 0xe3, {L3, UNIFIED,   4 MB, NONE, 0x10, 64}},
	{ 0xe4, {L3, UNIFIED,   8 MB, NONE, 0x10, 64}},
	{ 0xea, {L3, UNIFIED,  12 MB, NONE, 0x18, 64}},
	{ 0xeb, {L3, UNIFIED,  18 MB, NONE, 0x18, 64}},
	{ 0xec, {L3, UNIFIED,  24 MB, NONE, 0x18, 64}},
	
	/* Special cases, not described in this table. */
	{ 0x40, {0, 0, 0, 0, 0, 0}},
	{ 0xf0, {0, 0, 0, 0, 0, 0}},
	{ 0xf1, {0, 0, 0, 0, 0, 0}},

	{ 0x00, {0, 0, 0, 0, 0, 0}}
};
static const struct cache_desc_index_t descriptor_49[] = {
	{ 0x49, {L2, UNIFIED,  4 MB, NONE, 0x10, 64}},
	{ 0x49, {L3, UNIFIED,  4 MB, NONE, 0x10, 64}}
};
#undef MB

static const char *page_types(uint32_t attrs)
{
	/* There's probably a much better algorithm for this. */
	attrs &= (PAGES_4K | PAGES_2M | PAGES_4M);
	switch(attrs) {
	case 0:
		return NULL;
	case PAGES_4K:
		return "4KB pages";
	case PAGES_2M:
		return "2MB pages";
	case PAGES_4M:
		return "4MB pages";
	case PAGES_4K | PAGES_4M:
		return "4KB or 4MB pages";
	case PAGES_2M | PAGES_4M:
		return "2MB or 4MB pages";
	case PAGES_4K | PAGES_2M | PAGES_4M:
		return "4KB, 2MB, or 4MB pages";
	default:
		abort();
	}
#ifdef _MSC_VER
	/* Visual C++ isn't bright enough to figure out that
	 * all control paths DO return a value (or don't return at all).
	 */
	return NULL;
#endif
}

static const char *associativity(char *buffer, uint8_t assoc)
{
	switch(assoc) {
	case 0x00:
		return "unknown associativity";
	case 0x01:
		return "direct-mapped";
	case 0xFF:
		return "fully associative";
	}
	sprintf(buffer, "%d-way set associative", assoc);
	return buffer;
}

static const char *size(char *buffer, uint32_t size)
{
	if (size > 1024) {
		sprintf(buffer, "%dMB", size / 1024);
	} else {
		sprintf(buffer, "%dKB", size);
	}
	return buffer;
}

static char *create_description(const struct cache_desc_index_t *idx)
{
	const struct cache_desc_t *desc = &idx->desc;
	char buffer[128], temp[32];
	const char *cp;
	buffer[0] = 0;

	/* Special cases. */
	switch(idx->descriptor) {
	case 0x40:
		sprintf(buffer, "No L2 cache, or if L2 cache exists, no L3 cache");
		goto out;
	case 0xF0:
		sprintf(buffer, "64-byte prefetching");
		goto out;
	case 0xF1:
		sprintf(buffer, "64-byte prefetching");
		goto out;
	}

	switch(desc->level) {
	case NO:
		break;
	case L0:
		strcat(buffer, "L0 ");
		break;
	case L1:
		strcat(buffer, "L1 ");
		break;
	case L2:
		strcat(buffer, "L2 ");
		break;
	case L3:
		strcat(buffer, "L3 ");
		break;
	default:
		abort();
	}

	switch (desc->type) {
	case TRACE:
		strcat(buffer, "trace cache: ");
		sprintf(temp, "%dK-uops", desc->size);
		strcat(buffer, temp);
		break;
	case DATA_TLB:
		strcat(buffer, "Data TLB: ");
		break;
	case CODE_TLB:
		strcat(buffer, "Code TLB: ");
		break;
	case SHARED_TLB:
		strcat(buffer, "shared TLB: ");
		break;
	case CODE:
		strcat(buffer, "code cache: ");
		strcat(buffer, size(temp, desc->size));
		break;
	case DATA:
		strcat(buffer, "data cache: ");
		strcat(buffer, size(temp, desc->size));
		break;
	case UNIFIED:
		strcat(buffer, "cache: ");
		strcat(buffer, size(temp, desc->size));
		break;
	default:
		abort();
	}

	cp = page_types(desc->attrs);
	if (cp) {
		strcat(buffer, page_types(desc->attrs));
	}

	if (desc->assoc != 0) {
		strcat(buffer, ", ");
		strcat(buffer, associativity(temp, desc->assoc));
	}

	if (desc->attrs & SECTORED)
		strcat(buffer, ", sectored cache");

	switch (desc->type) {
	case CODE:
	case DATA:
	case UNIFIED:
		sprintf(temp, ", %d byte line size", desc->linesize);
		strcat(buffer, temp);
		break;
	case DATA_TLB:
	case CODE_TLB:
	case SHARED_TLB:
		sprintf(temp, ", %d entries", desc->size);
		strcat(buffer, temp);
		break;
	default:
		break;
	}

	if (desc->attrs & ECC)
		strcat(buffer, ", ECC");

	if (desc->attrs & UNDOCUMENTED)
		strcat(buffer, " (undocumented)");
out:
	return strdup(buffer);
}

static int entry_comparator(const void *a, const void *b)
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
	char *entries[17];

	char **eptr = entries;

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
		char *desc = NULL;
		const struct cache_desc_index_t *d;

		if (buf[i] == 0)
			continue;

		for(d = descs; d->descriptor; d++) {
			if (d->descriptor == buf[i])
				break;
		}

		if (d->descriptor)
			desc = create_description(d);

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
				          create_description(&descriptor_49[1]) :
				          create_description(&descriptor_49[0]);
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
		free(*eptr);
		eptr++;
	}
}

