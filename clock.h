/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2020, Steven Noonan <steven@uplinklabs.net>
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

#ifndef __clock_h
#define __clock_h

#ifdef _MSC_VER
#include <intrin.h>
#endif

static inline uint64_t get_cpu_clock(void)
{
#if defined(TARGET_CPU_X86) || defined(TARGET_CPU_X86_64)
#ifdef _MSC_VER
    return __rdtsc();
#else
    uint32_t lo, hi;

    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t) hi << 32ULL) | lo;
#endif
#else
	/* This shouldn't be executed on non-x86 */
	assert(0);
	return 0;
#endif
}

uint64_t cpu_clock_to_wall(uint64_t clock);
void init_cpu_clock(void);

#endif

/* vim: set ts=4 sts=4 sw=4 noet: */
