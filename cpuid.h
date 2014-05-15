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

#ifndef __cpuid_h
#define __cpuid_h

struct cpu_regs_t {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

#define ZERO_REGS(x) {memset((x), 0, sizeof(struct cpu_regs_t));}

BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx);

struct cpuid_state_t;

typedef BOOL(*cpuid_call_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *);
typedef void(*cpuid_print_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *, BOOL);

/* Makes a lot of calls easier to do. */
BOOL cpuid_native(struct cpu_regs_t *regs, struct cpuid_state_t *state);
#ifdef __linux__
BOOL cpuid_kernel(struct cpu_regs_t *regs, struct cpuid_state_t *state);
#endif
BOOL cpuid_stub(struct cpu_regs_t *regs, struct cpuid_state_t *state);

/* Allows printing dumps in different formats. */
void cpuid_dump_normal(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_xen(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_xen_sxp(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_etallen(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_vmware(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);

/* For cpuid_pseudo */
BOOL cpuid_load_from_file(const char *filename, struct cpuid_state_t *state);

#endif
