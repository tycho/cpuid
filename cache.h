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

#ifndef __cache_h
#define __cache_h

struct cpu_regs_t;
struct cpu_signature_t;

void print_intel_caches(struct cpu_regs_t *regs, const struct cpu_signature_t *sig);

typedef enum {
	DATA_TLB = 0x0,
	CODE_TLB,
	SHARED_TLB,
	LOADONLY_TLB,
	STOREONLY_TLB,
	DATA,
	CODE,
	UNIFIED,
	TRACE,
	INVALID_TYPE = 0xff,
} cache_type_t;

typedef enum {
	NO = 0xfe,
	L0 = 0x0,
	L1 = 0x1,
	L2 = 0x2,
	L3 = 0x3,
	L4 = 0x4,
	LMAX = 0xf, /* unlikely, but just cautious */
	INVALID_LEVEL = 0xff,
} cache_level_t;

typedef enum {
	NONE = 0x0,
	UNDOCUMENTED = 0x1,
	IA64 = 0x2,
	ECC = 0x4,
	SECTORED = 0x8,
	PAGES_4K = 0x10,
	PAGES_2M = 0x20,
	PAGES_4M = 0x40,
	PAGES_1G = 0x80,
	SELF_INIT = 0x100,
	CPLX_INDEX = 0x200,
	INCLUSIVE = 0x400,
	WBINVD_NOT_INCLUSIVE = 0x800,
} extra_attrs_t;

struct cache_desc_t {
	cache_level_t level;
	cache_type_t type;
	uint32_t size;
	uint32_t attrs;
	uint8_t assoc;
	uint8_t linesize;
	uint16_t partitions;
	uint16_t max_threads_sharing;
};

char *describe_cache(uint32_t ncpus, const struct cache_desc_t *desc, char *buffer, size_t bufsize, int indent);

#endif

/* vim: set ts=4 sts=4 sw=4 noet: */
