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

#ifndef __threads_h
#define __threads_h

struct cpuid_state_t;

typedef int (*thread_bind_handler_t)(struct cpuid_state_t *, uint32_t);
typedef uint32_t (*thread_count_handler_t)(struct cpuid_state_t *);

/* These do real thread binding. */
int thread_bind_native(struct cpuid_state_t *state, uint32_t id);
uint32_t thread_count_native(struct cpuid_state_t *state);

/* These are used to change the logical selector in the state structure. */
int thread_bind_stub(struct cpuid_state_t *state, uint32_t id);
uint32_t thread_count_stub(struct cpuid_state_t *state);

/* These aren't used during CPUID runs, just in sanity check runs. */
uintptr_t thread_get_binding(void);
uintptr_t thread_bind_mask(uintptr_t mask);

#endif
