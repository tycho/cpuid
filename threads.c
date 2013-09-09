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

#include <stdio.h>

#define MAX_CPUS 1024

#ifdef TARGET_OS_LINUX

#include <pthread.h>
#include <sched.h>
#include <string.h>
#define CPUSET_T cpu_set_t
#define CPUSET_MASK_T __cpu_mask

#elif defined(TARGET_OS_FREEBSD)

#include <pthread.h>
#include <pthread_np.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#define CPUSET_T cpuset_t
#define CPUSET_MASK_T long
#undef MAX_CPUS
#define MAX_CPUS CPU_MAXSIZE

#elif defined(TARGET_OS_MACOSX)

#include <sys/sysctl.h>

/*#define USE_CHUD*/
#ifdef USE_CHUD
extern int chudProcessorCount(void);
extern int utilBindThreadToCPU(int n);
extern int utilUnbindThreadFromCPU(void);
#endif

#endif

#include "state.h"

uint32_t thread_count_native(struct cpuid_state_t *state)
{
#ifdef TARGET_OS_MACOSX
	uint32_t count;
	size_t  size = sizeof(count);

	if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0))
		return 1;

	return count;
#else
	static unsigned int i = 0;
	if (i) return i;
	if (thread_bind_native(state, 0) != 0) {
		fprintf(stderr, "ERROR: thread_bind() doesn't appear to be working correctly.\n");
		abort();
	}
	while (thread_bind_native(state, ++i) == 0);
	return i;
#endif
}

uint32_t thread_count_stub(struct cpuid_state_t *state)
{
	assert(state);
	return state->cpu_logical_count;
}

uintptr_t thread_get_binding(void)
{
#ifdef TARGET_OS_WINDOWS

	HANDLE hThread = GetCurrentThread();
	DWORD_PTR mask = SetThreadAffinityMask(hThread, 1);
	SetThreadAffinityMask(hThread, mask);

	return mask;

#elif defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)

	uint32_t ret, mask_id;
	CPUSET_T mask;
	pthread_t pth;

	pth = pthread_self();
	CPU_ZERO(&mask);
	ret = pthread_getaffinity_np(pth, sizeof(mask), &mask);
	if (ret != 0) {
		fprintf(stderr, "ERROR: pthread_getaffinity_np() failed (returned %d)\n", ret);
		abort();
	}

	mask_id = 0;
	for (ret = 0; ret < 32; ret++) {
		if (CPU_ISSET(ret, &mask))
			mask_id |= (1 << ret);
	}

	return mask_id;

#elif defined(TARGET_OS_MACOSX)

	/* Hopefully this function wasn't too important. */
	return 0;

#else
#error "thread_get_binding() not defined for this platform"
#endif
}

uintptr_t thread_bind_mask(uintptr_t _mask)
{
#ifdef TARGET_OS_WINDOWS

	DWORD ret;
	HANDLE hThread = GetCurrentThread();
	ret = SetThreadAffinityMask(hThread, (DWORD_PTR)_mask);

	return (ret != 0) ? 0 : 1;

#elif defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)

	int ret;
	CPUSET_T mask;
	pthread_t pth;

	pth = pthread_self();

	CPU_ZERO(&mask);
	for (ret = 0; ret < 64; ret++) {
		if (_mask & 1)
			CPU_SET(ret, &mask);
		_mask >>= 1;
	}

	ret = pthread_setaffinity_np(pth, sizeof(mask), &mask);

	return (ret == 0) ? 0 : 1;

#elif defined(TARGET_OS_MACOSX)

	/* We just have to pray nothing explodes too dramatically. */
	return 0;

#else
#error "thread_bind_mask() not defined for this platform"
#endif
}

int thread_bind_native(__unused_variable struct cpuid_state_t *state, uint32_t id)
{
#ifdef TARGET_OS_WINDOWS

	BOOL ret;
	HANDLE hThread = GetCurrentThread();
#if _WIN32_WINNT >= 0x0601
	GROUP_AFFINITY affinity;

	ZeroMemory(&affinity, sizeof(GROUP_AFFINITY));

	affinity.Group = id / 64;
	affinity.Mask = 1 << (id % 64);

	ret = SetThreadGroupAffinity(hThread, &affinity, NULL);
#else
	DWORD mask;

	if (id > 32)
		return 1;

	mask = (1 << id);

	ret = SetThreadAffinityMask(hThread, mask);
#endif

	return (ret != FALSE) ? 0 : 1;

#elif defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)

	int ret;

#ifdef CPU_SET_S
	size_t setsize = CPU_ALLOC_SIZE(MAX_CPUS);
	CPUSET_T *set = CPU_ALLOC(MAX_CPUS);
	pthread_t pth;

	pth = pthread_self();

	CPU_ZERO_S(setsize, set);
	CPU_SET_S(id, setsize, set);
	ret = pthread_setaffinity_np(pth, setsize, set);
	CPU_FREE(set);
#else
	size_t bits_per_set = sizeof(CPUSET_T) * 8;
	size_t bits_per_subset = sizeof(CPUSET_MASK_T) * 8;
	size_t setsize = sizeof(CPUSET_T) * (MAX_CPUS / bits_per_set);
	size_t set_id, subset_id;
	unsigned long long mask;
	CPUSET_T *set = malloc(setsize);
	pthread_t pth;

	pth = pthread_self();

	for (set_id = 0; set_id < (MAX_CPUS / bits_per_set); set_id++)
		CPU_ZERO(&set[set_id]);

	set_id = id / bits_per_set;
	id %= bits_per_set;

	subset_id = id / bits_per_subset;
	id %= bits_per_subset;

	mask = 1ULL << (unsigned long long)id;

	((unsigned long *)set[set_id].__bits)[subset_id] |= mask;
	ret = pthread_setaffinity_np(pth, setsize, set);
	free(set);
#endif

	return (ret == 0) ? 0 : 1;

#elif defined(TARGET_OS_MACOSX)

#ifdef USE_CHUD
	return (utilBindThreadToCPU(id) == 0) ? 0 : 1;
#else
	return 1;
#endif

#else
#error "thread_bind_native() not defined for this platform"
#endif
}

int thread_bind_stub(struct cpuid_state_t *state, uint32_t id)
{
	assert(state);
	assert(id < state->cpu_logical_count);
	state->cpu_bound_index = id;
	return 0;
}
