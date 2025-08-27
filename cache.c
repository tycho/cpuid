/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2025, Steven Noonan <steven@uplinklabs.net>
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
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cache_desc_index_t {
	uint8_t descriptor;
	struct cache_desc_t desc;
};

#define MB * 1024
static const struct cache_desc_index_t descs[] = {
	{ 0x01, {NO, CODE_TLB,    32, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0x02, {NO, CODE_TLB,     2, PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x03, {NO, DATA_TLB,    64, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0x04, {NO, DATA_TLB,     8, PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x05, {NO, DATA_TLB,    32, PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x06, {L1, CODE,         8, NONE, 0x04, 32, 0, 0} },
	{ 0x08, {L1, CODE,        16, NONE, 0x04, 32, 0, 0} },
	{ 0x09, {L1, CODE,        32, NONE, 0x04, 64, 0, 0} },
	{ 0x0a, {L1, DATA,         8, NONE, 0x02, 32, 0, 0} },
	{ 0x0b, {L1, CODE_TLB,     4, PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x0c, {L1, DATA,        16, NONE, 0x04, 32, 0, 0} },
	{ 0x0d, {L1, DATA,        16, ECC, 0x04, 64, 0, 0} },
	{ 0x0e, {L1, DATA,        24, NONE, 0x06, 64, 0, 0} },
	{ 0x10, {L1, DATA,        16, IA64, 0x04, 32, 0, 0} },
	{ 0x15, {L1, CODE,        16, IA64, 0x04, 32, 0, 0} },
	{ 0x1a, {L2, UNIFIED,     96, IA64, 0x06, 64, 0, 0} },
	{ 0x1d, {L2, UNIFIED,    128, NONE, 0x02, 64, 0, 0} },
	{ 0x21, {L2, UNIFIED,    256, NONE, 0x08, 64, 0, 0} },
	{ 0x22, {L3, UNIFIED,    512, SECTORED, 0x04, 64, 0, 0} },
	{ 0x23, {L3, UNIFIED,   1 MB, SECTORED, 0x08, 64, 0, 0} },
	{ 0x24, {L2, UNIFIED,   1 MB, NONE, 0x10, 64, 0, 0} },
	{ 0x25, {L3, UNIFIED,   2 MB, SECTORED, 0x08, 64, 0, 0} },
	{ 0x29, {L3, UNIFIED,   4 MB, SECTORED, 0x08, 64, 0, 0} },
	{ 0x2c, {L1, DATA,        32, NONE, 0x08, 64, 0, 0} },
	{ 0x30, {L1, CODE,        32, NONE, 0x08, 64, 0, 0} },
	{ 0x39, {L2, UNIFIED,    128, SECTORED, 0x04, 64, 0, 0} },
	{ 0x3a, {L2, UNIFIED,    192, SECTORED, 0x06, 64, 0, 0} },
	{ 0x3b, {L2, UNIFIED,    128, SECTORED, 0x02, 64, 0, 0} },
	{ 0x3c, {L2, UNIFIED,    256, SECTORED, 0x04, 64, 0, 0} },
	{ 0x3d, {L2, UNIFIED,    384, SECTORED, 0x06, 64, 0, 0} },
	{ 0x3e, {L2, UNIFIED,    512, SECTORED, 0x04, 64, 0, 0} },
	{ 0x40, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} }, /* Special case, see create_description() */
	{ 0x41, {L2, UNIFIED,    128, NONE, 0x04, 32, 0, 0} },
	{ 0x42, {L2, UNIFIED,    256, NONE, 0x04, 32, 0, 0} },
	{ 0x43, {L2, UNIFIED,    512, NONE, 0x04, 32, 0, 0} },
	{ 0x44, {L2, UNIFIED,   1 MB, NONE, 0x04, 32, 0, 0} },
	{ 0x45, {L2, UNIFIED,   2 MB, NONE, 0x04, 32, 0, 0} },
	{ 0x46, {L3, UNIFIED,   4 MB, NONE, 0x04, 64, 0, 0} },
	{ 0x47, {L3, UNIFIED,   8 MB, NONE, 0x08, 64, 0, 0} },
	{ 0x48, {L2, UNIFIED,   3 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0x4a, {L3, UNIFIED,   6 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0x4b, {L3, UNIFIED,   8 MB, NONE, 0x10, 64, 0, 0} },
	{ 0x4c, {L3, UNIFIED,  12 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0x4d, {L3, UNIFIED,  16 MB, NONE, 0x10, 64, 0, 0} },
	{ 0x4e, {L2, UNIFIED,   6 MB, NONE, 0x18, 64, 0, 0} },
	{ 0x4f, {NO, CODE_TLB,    32, PAGES_4K, 0x00, 0, 0, 0} },
	{ 0x50, {NO, CODE_TLB,    64, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0, 0, 0} },
	{ 0x51, {NO, CODE_TLB,   128, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0, 0, 0} },
	{ 0x52, {NO, CODE_TLB,   256, PAGES_4K | PAGES_2M | PAGES_4M, 0x00, 0, 0, 0} },
	{ 0x55, {NO, CODE_TLB,   256, PAGES_2M | PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x56, {L0, DATA_TLB,    16, PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x57, {L0, DATA_TLB,    16, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0x59, {L0, DATA_TLB,    16, PAGES_4K, 0xFF, 0, 0, 0} },
	{ 0x5a, {NO, DATA_TLB,    32, PAGES_2M | PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x5b, {NO, DATA_TLB,    64, PAGES_4K | PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x5c, {NO, DATA_TLB,   128, PAGES_4K | PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x5d, {NO, DATA_TLB,   256, PAGES_4K | PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x60, {L1, DATA,        16, SECTORED, 0x08, 64, 0, 0} },
	{ 0x61, {NO, CODE_TLB,    48, PAGES_4K, 0xFF, 0, 0, 0} },
	{ 0x63, {NO, DATA_TLB,    32, PAGES_2M | PAGES_4M, 0x04, 0, 0, 0} },
	{ 0x63, {NO, DATA_TLB,     4, PAGES_1G, 0x04, 0, 0, 0} },
	{ 0x64, {NO, DATA_TLB,   512, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0x66, {L1, DATA,         8, SECTORED, 0x04, 64, 0, 0} },
	{ 0x67, {L1, DATA,        16, SECTORED, 0x04, 64, 0, 0} },
	{ 0x68, {L1, DATA,        32, SECTORED, 0x04, 64, 0, 0} },
	{ 0x6a, {L0, DATA_TLB,    64, PAGES_4K, 0x08, 0, 0, 0} },
	{ 0x6b, {NO, DATA_TLB,   256, PAGES_4K, 0x08, 0, 0, 0} },
	{ 0x6c, {NO, DATA_TLB,   128, PAGES_2M | PAGES_4M, 0x08, 0, 0, 0} },
	{ 0x6d, {NO, DATA_TLB,    16, PAGES_1G, 0xFF, 0, 0, 0} },
	{ 0x70, {L1, TRACE,       12, NONE, 0x08, 0, 0, 0} },
	{ 0x71, {L1, TRACE,       16, NONE, 0x08, 0, 0, 0} },
	{ 0x72, {L1, TRACE,       32, NONE, 0x08, 0, 0, 0} },
	{ 0x73, {L1, TRACE,       64, UNDOCUMENTED, 0x08, 0, 0, 0} },
	{ 0x76, {NO, CODE_TLB,     8, PAGES_2M | PAGES_4M, 0xFF, 0, 0, 0} },
	{ 0x77, {L1, CODE,        16, SECTORED | IA64, 0x04, 64, 0, 0} },
	{ 0x78, {L2, UNIFIED,   1 MB, NONE, 0x04, 64, 0, 0} },
	{ 0x79, {L2, UNIFIED,    128, SECTORED, 0x08, 64, 0, 0} },
	{ 0x7a, {L2, UNIFIED,    256, SECTORED, 0x04, 64, 0, 0} },
	{ 0x7b, {L2, UNIFIED,    512, SECTORED, 0x04, 64, 0, 0} },
	{ 0x7c, {L2, UNIFIED,   1 MB, SECTORED, 0x04, 64, 0, 0} },
	{ 0x7d, {L2, UNIFIED,   2 MB, NONE, 0x08, 64, 0, 0} },
	{ 0x7e, {L2, UNIFIED,    256, SECTORED | IA64, 0x08, 128, 0, 0} },
	{ 0x7f, {L2, UNIFIED,    512, NONE, 0x02, 64, 0, 0} },
	{ 0x80, {L2, UNIFIED,    512, NONE, 0x08, 64, 0, 0} },
	{ 0x81, {L2, UNIFIED,    128, UNDOCUMENTED, 0x08, 32, 0, 0} },
	{ 0x82, {L2, UNIFIED,    256, NONE, 0x08, 32, 0, 0} },
	{ 0x83, {L2, UNIFIED,    512, NONE, 0x08, 32, 0, 0} },
	{ 0x84, {L2, UNIFIED,   1 MB, NONE, 0x08, 32, 0, 0} },
	{ 0x85, {L2, UNIFIED,   2 MB, NONE, 0x08, 32, 0, 0} },
	{ 0x86, {L2, UNIFIED,    512, NONE, 0x04, 64, 0, 0} },
	{ 0x87, {L2, UNIFIED,   1 MB, NONE, 0x08, 64, 0, 0} },
	{ 0x88, {L3, UNIFIED,   2 MB, IA64, 0x04, 64, 0, 0} },
	{ 0x89, {L3, UNIFIED,   4 MB, IA64, 0x04, 64, 0, 0} },
	{ 0x8a, {L3, UNIFIED,   8 MB, IA64, 0x04, 64, 0, 0} },
	{ 0x8d, {L3, UNIFIED,   3 MB, IA64, 0x0C, 128, 0, 0} },
	{ 0xa0, {NO, DATA_TLB,    32, PAGES_4K, 0xFF, 0, 0, 0} },
	{ 0xb0, {NO, CODE_TLB,   128, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xb1, {NO, CODE_TLB,     4, PAGES_4M, 0x04, 0, 0, 0} },
	{ 0xb1, {NO, CODE_TLB,     8, PAGES_2M, 0x04, 0, 0, 0} },
	{ 0xb2, {NO, DATA_TLB,    64, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xb3, {NO, DATA_TLB,   128, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xb4, {L1, DATA_TLB,   256, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xb5, {NO, CODE_TLB,    64, PAGES_4K, 0x08, 0, 0, 0} },
	{ 0xb6, {NO, CODE_TLB,   128, PAGES_4K, 0x08, 0, 0, 0} },
	{ 0xba, {L1, DATA_TLB,    64, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xc0, {NO, DATA_TLB,     8, PAGES_4K | PAGES_4M, 0x04, 0, 0, 0} },
	{ 0xc1, {L2, SHARED_TLB,1024, PAGES_4K | PAGES_2M, 0x08, 0, 0, 0} },
	{ 0xc2, {NO, DATA_TLB,    16, PAGES_2M | PAGES_4M, 0x04, 0, 0, 0} },
	{ 0xc3, {L2, SHARED_TLB,1536, PAGES_4K | PAGES_2M, 0x04, 0, 0, 0} },
	{ 0xc3, {L2, SHARED_TLB,  16, PAGES_1G, 0x04, 0, 0, 0} },
	{ 0xc4, {NO, DATA_TLB,    32, PAGES_2M | PAGES_4M, 0x04, 0, 0, 0} },
	{ 0xca, {L2, SHARED_TLB, 512, PAGES_4K, 0x04, 0, 0, 0} },
	{ 0xd0, {L3, UNIFIED,    512, NONE, 0x04, 64, 0, 0} },
	{ 0xd1, {L3, UNIFIED,   1 MB, NONE, 0x04, 64, 0, 0} },
	{ 0xd2, {L3, UNIFIED,   2 MB, NONE, 0x04, 64, 0, 0} },
	{ 0xd6, {L3, UNIFIED,   1 MB, NONE, 0x08, 64, 0, 0} },
	{ 0xd7, {L3, UNIFIED,   2 MB, NONE, 0x08, 64, 0, 0} },
	{ 0xd8, {L3, UNIFIED,   4 MB, NONE, 0x08, 64, 0, 0} },
	{ 0xdc, {L3, UNIFIED, 1.5 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0xdd, {L3, UNIFIED,   3 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0xde, {L3, UNIFIED,   6 MB, NONE, 0x0C, 64, 0, 0} },
	{ 0xe2, {L3, UNIFIED,   2 MB, NONE, 0x10, 64, 0, 0} },
	{ 0xe3, {L3, UNIFIED,   4 MB, NONE, 0x10, 64, 0, 0} },
	{ 0xe4, {L3, UNIFIED,   8 MB, NONE, 0x10, 64, 0, 0} },
	{ 0xea, {L3, UNIFIED,  12 MB, NONE, 0x18, 64, 0, 0} },
	{ 0xeb, {L3, UNIFIED,  18 MB, NONE, 0x18, 64, 0, 0} },
	{ 0xec, {L3, UNIFIED,  24 MB, NONE, 0x18, 64, 0, 0} },

	/* Special cases, not described in this table, but handled in the
	 * create_description() function. */
	{ 0xf0, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} },
	{ 0xf1, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} },
	{ 0xfe, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} },
	{ 0xff, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} },

	{ 0x00, {INVALID_LEVEL, INVALID_TYPE, 0, 0, 0, 0, 0, 0} }
};
static const struct cache_desc_index_t descriptor_49[] = {
	{ 0x49, {L2, UNIFIED,  4 MB, NONE, 0x10, 64, 0, 0} },
	{ 0x49, {L3, UNIFIED,  4 MB, NONE, 0x10, 64, 0, 0} }
};
#undef MB

#define DELIM() { \
		if (rem_types >= 2 && num_types >= 3) { \
			safe_strcat(buffer, ", ", sizeof(buffer)); \
		} else if (num_types >= 2 && rem_types < num_types) { \
			safe_strcat(buffer, " or ", sizeof(buffer)); \
		} \
	}
#define ADD_TYPE(type) { \
		safe_strcat(buffer, type, sizeof(buffer)); \
		rem_types--; \
	}
static const char *page_types(uint32_t attrs)
{
	uint32_t num_types, rem_types;
	static char buffer[48];

	buffer[0] = 0;

	attrs &= (PAGES_4K | PAGES_2M | PAGES_4M | PAGES_1G);
	num_types = popcnt(attrs);
	rem_types = num_types;

	if (attrs & PAGES_4K) {
		ADD_TYPE("4KB");
	}

	if (attrs & PAGES_2M) {
		DELIM();
		ADD_TYPE("2MB");
	}

	if (attrs & PAGES_4M) {
		DELIM();
		ADD_TYPE("4MB");
	}

	if (attrs & PAGES_1G) {
		DELIM();
		ADD_TYPE("1GB");
	}

	strcat(buffer, " pages");
	return buffer;
}
#undef DELIM
#undef ADD_TYPE

static const char *type(cache_type_t type)
{
	switch(type) {
	case DATA_TLB:       return "Data TLB";
	case CODE_TLB:       return "Code TLB";
	case SHARED_TLB:     return "Shared TLB";
	case LOADONLY_TLB:   return "Load-only TLB";
	case STOREONLY_TLB:  return "Store-only TLB";
	case DATA:           return "data cache";
	case CODE:           return "code cache";
	case UNIFIED:        return "unified cache";
	case TRACE:          return "trace cache";
	default:             abort();
	}
	return NULL;
}

static const char *level(cache_level_t level)
{
	static char buffer[8];
	buffer[0] = 0;
	if (level == INVALID_LEVEL || level > LMAX)
		return NULL;
	if (level == NO)
		return buffer;
	sprintf(buffer, "L%d", (int)level);
	return buffer;
}

static const char *associativity(uint8_t assoc)
{
	static char buffer[32];
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

static const char *size(uint32_t size)
{
	static char buffer[16];
	if (size >= 1024) {
		sprintf(buffer, "%dMB", size / 1024);
	} else {
		sprintf(buffer, "%dKB", size);
	}
	return buffer;
}

#define ADD_LINE(fmt, ...) { \
		sprintf(temp, "%*s" fmt "\n", indent, "", __VA_ARGS__); \
		safe_strcat(buffer, temp, bufsize); \
	}

char *describe_cache(uint32_t ncpus, const struct cache_desc_t *desc, char *buffer, size_t bufsize, int indent)
{
	char temp[64], temp1[32];
	uint32_t instances = 0;

	buffer[0] = 0;

	/* If we know ncpus and max_threads_sharing, we can make an educated guess
	 * about the number of instances of this particular cache in the socket.
	 */
	if (ncpus)
		instances = 1;
	if (ncpus && desc->max_threads_sharing && ncpus > desc->max_threads_sharing)
		instances = ncpus / desc->max_threads_sharing;

	switch (desc->type) {
	case DATA_TLB:
	case CODE_TLB:
	case SHARED_TLB:
	case LOADONLY_TLB:
	case STOREONLY_TLB:
		/* e.g. "Code TLB: 2MB or 4MB pages" */
		if (desc->level != NO) {
			sprintf(temp1, "%s %s", level(desc->level), type(desc->type));
			ADD_LINE("%17s: %s",
				temp1,
				page_types(desc->attrs));
		} else {
			ADD_LINE("%17s: %s",
				type(desc->type),
				page_types(desc->attrs));
		}
		indent += 19;
		break;
	case CODE:
	case DATA:
	case UNIFIED:
		if (instances) {
			/* e.g. "16 x 32KB L1 data cache" */
			ADD_LINE("%2d x %5s %s %s",
				instances,
				size(desc->size),
				level(desc->level),
				type(desc->type));
			indent += 11;
		} else {
			/* e.g. "32KB L1 data cache" */
			ADD_LINE("%5s %s %s",
				size(desc->size),
				level(desc->level),
				type(desc->type));
			indent += 6;
		}
		break;
	case TRACE:
		ADD_LINE("%dK-uops trace cache",
			desc->size);
		indent += 6;
		break;
	default:
		abort();
	}

	if (desc->assoc != 0) {
		/* e.g. "8-way set associative" */
		ADD_LINE("%s", associativity(desc->assoc));
	}

	if (desc->attrs & SECTORED) {
		ADD_LINE("%s", "Sectored cache");
	}

	switch (desc->type) {
	case CODE:
	case DATA:
	case UNIFIED:
		if (desc->partitions > 1) {
			ADD_LINE("%d byte line size (%d partitions)", desc->linesize * desc->partitions, desc->partitions);
		} else {
			ADD_LINE("%d byte line size", desc->linesize);
		}
		break;
	case DATA_TLB:
	case CODE_TLB:
	case SHARED_TLB:
		ADD_LINE("%d entries", desc->size);
		break;
	default:
		break;
	}

	if (desc->attrs & ECC) {
		ADD_LINE("%s", "ECC");
	}

	if (desc->attrs & SELF_INIT) {
		ADD_LINE("%s", "Self-initializing");
	}

	if (desc->attrs & INCLUSIVE) {
		ADD_LINE("%s", "Inclusive of lower cache levels");
	}

	if (desc->attrs & CPLX_INDEX) {
		ADD_LINE("%s", "Complex indexing");
	}

	if (desc->attrs & WBINVD_NOT_INCLUSIVE) {
		ADD_LINE("%s", "Does not invalidate lower level caches");
	}

	if (desc->attrs & UNDOCUMENTED) {
		ADD_LINE("%s", "Undocumented descriptor");
	}

	if (desc->max_threads_sharing) {
		ADD_LINE("Shared by max %d threads", desc->max_threads_sharing);
	}

	return buffer;
}
#undef ADD_LINE

static char *create_description(const struct cache_desc_index_t *idx)
{
	const struct cache_desc_t *desc = &idx->desc;
	char buffer[256];

#if 0
	/* For debugging */
	sprintf(buffer, "0x%02x: ", idx->descriptor);
#endif

	/* Special cases. */
	switch(idx->descriptor) {
	case 0x40: return strdup("  No L2 cache, or if L2 cache exists, no L3 cache");
	case 0xF0: return strdup("  64-byte prefetching");
	case 0xF1: return strdup("  128-byte prefetching");
	case 0xFE: return strdup("  [NOTICE] For TLB data, see Deterministic Address Translation leaf instead");
	case 0xFF: return strdup("  [NOTICE] For cache data, see Deterministic Cache Parameters leaf instead");
	}

	describe_cache(0, desc, buffer, sizeof(buffer), 2);
	return strdup(buffer);
}

static int entry_comparator(const void *a, const void *b)
{
	const char *stra = *(const char **)a;
	const char *strb = *(const char **)b;

	/* Make notices appear first. */
	if ((stra)[strspn(stra, " ")] == '[') return -1;
	if ((strb)[strspn(strb, " ")] == '[') return 1;

	/* Make 'prefetching' lines show last. */
	if (strstr(*(const char **)a, "prefetch")) return 1;
	if (strstr(*(const char **)b, "prefetch")) return -1;

	/* Simple string compare. */
	return strcmp(*(const char **)a, *(const char **)b);
}

#define MAX_ENTRIES 32
void print_intel_caches(struct cpu_regs_t *regs, const struct cpu_signature_t *sig)
{
	uint8_t buf[16] ALIGNED(4);
	uint8_t last_descriptor = 0;
	uint32_t i;

	/* It's only possible to have 16 entries on a single line, but some
	 * descriptors have two descriptions tied to them, so support up to 32
	 * strings for a single line.
	 */
	char *entries[MAX_ENTRIES + 1];
	char **eptr = entries;

	entries[MAX_ENTRIES] = 0;

	memset(buf, 0, sizeof(buf));

	*(uint32_t *)&buf[0x0] = regs->eax >> 8;
	if ((regs->ebx & (1U << 31)) == 0)
		*(uint32_t *)&buf[0x4] = regs->ebx;
	if ((regs->ecx & (1U << 31)) == 0)
		*(uint32_t *)&buf[0x8] = regs->ecx;
	if ((regs->edx & (1U << 31)) == 0)
		*(uint32_t *)&buf[0xC] = regs->edx;

	for (i = 0; i <= 0xF; i++) {
		BOOL found_match = FALSE;
		char *desc = NULL;
		const struct cache_desc_index_t *d;

		if (buf[i] == 0)
			continue;

		if (buf[i] == last_descriptor)
			continue;

		last_descriptor = buf[i];

		if (buf[i] == 0x49) {
			/* A very stupid special case. AP-485 says this
			 * is a L3 cache for Intel Xeon processor MP,
			 * Family 0Fh, Model 06h, while it's a L2 cache
			 * on everything else.
			 */
			*eptr++ = (sig->family == 0x0F && sig->model == 0x06) ?
					  create_description(&descriptor_49[1]) :
					  create_description(&descriptor_49[0]);
			continue;
		}

		for(d = descs; d->descriptor; d++) {
			/* Descriptor table is ordered, and may have multiple entries for
			 * a specific identifier.
			 */
			if (d->descriptor > buf[i])
				break;
			if (d->descriptor < buf[i])
				continue;

			found_match = TRUE;

			desc = create_description(d);

			*eptr++ = desc;
		}

		if (!found_match) {
			/* This one we can just print right away. Its exact string
			   will vary, and we wouldn't know how to sort it anyway. */
			printf("  Unknown cache descriptor (0x%02x)\n", buf[i]);
		}
	}

	i = (uint32_t)(eptr - entries);

	for(; eptr < &entries[MAX_ENTRIES]; eptr++) {
		*eptr = 0;
	}

	/* Sort alphabetically. Makes it a heck of a lot easier to read. */
	qsort(entries, i, sizeof(const char *), entry_comparator);

	/* Print the entries. */
	eptr = entries;
	while (*eptr) {
		printf("%s\n", *eptr);
		free(*eptr);
		eptr++;
	}
	printf("\n");
}

/* vim: set ts=4 sts=4 sw=4 noet: */
