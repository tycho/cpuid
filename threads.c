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

#elif defined(TARGET_OS_NETBSD)

#include <pthread.h>
#include <sched.h>

#elif defined(TARGET_OS_SOLARIS)
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>


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
#include "util.h"

#ifdef TARGET_OS_WINDOWS
#if _WIN32_WINNT < 0x0601
typedef WORD(WINAPI *fnGetActiveProcessorGroupCount)(void);
typedef DWORD(WINAPI *fnGetActiveProcessorCount)(WORD);
typedef BOOL(WINAPI *fnSetThreadGroupAffinity)(HANDLE, const GROUP_AFFINITY *, PGROUP_AFFINITY);

static fnGetActiveProcessorGroupCount GetActiveProcessorGroupCount;
static fnGetActiveProcessorCount GetActiveProcessorCount;
static fnSetThreadGroupAffinity SetThreadGroupAffinity;
#endif
#endif

void thread_init_native(void)
{
#ifdef TARGET_OS_WINDOWS
#if _WIN32_WINNT < 0x0601
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
	GetActiveProcessorGroupCount = (fnGetActiveProcessorGroupCount)GetProcAddress(hKernel32, "GetActiveProcessorGroupCount");
	GetActiveProcessorCount = (fnGetActiveProcessorCount)GetProcAddress(hKernel32, "GetActiveProcessorCount");
	SetThreadGroupAffinity = (fnSetThreadGroupAffinity)GetProcAddress(hKernel32, "SetThreadGroupAffinity");
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
#endif
}

uint32_t thread_count_native(struct cpuid_state_t *state)
{
#if defined(TARGET_OS_MACOSX)
	uint32_t count;
	size_t  size = sizeof(count);

	if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0))
		return 1;

	return count;
#elif defined(TARGET_OS_SOLARIS)
	long count;

	if ((count = sysconf(_SC_NPROCESSORS_ONLN)) == -1)
		return 1;

	(void)state;

	return (uint32_t)count;
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

void thread_init_stub(void)
{
}

uint32_t thread_count_stub(struct cpuid_state_t *state)
{
	assert(state);
	return state->cpu_logical_count;
}

int thread_bind_native(__unused_variable struct cpuid_state_t *state, uint32_t id)
{
#ifdef TARGET_OS_WINDOWS

	BOOL ret = FALSE;
	HANDLE hThread = GetCurrentThread();

#if _WIN32_WINNT < 0x0601
	if (is_windows7_or_greater()) {
#endif
		DWORD threadsInGroup = 0;
		WORD groupId, groupCount;
		GROUP_AFFINITY affinity;
		ZeroMemory(&affinity, sizeof(GROUP_AFFINITY));

		groupCount = GetActiveProcessorGroupCount();

		for (groupId = 0; groupId < groupCount; groupId++) {
			threadsInGroup = GetActiveProcessorCount(groupId);
			if (id < threadsInGroup)
				break;
			id -= threadsInGroup;
		}

		if (groupId < groupCount && id < threadsInGroup) {
			affinity.Group = groupId;
			affinity.Mask = 1ULL << id;

			ret = SetThreadGroupAffinity(hThread, &affinity, NULL);
		}
#if _WIN32_WINNT < 0x0601
	} else {
		DWORD mask;

		if (id > 32)
			return 1;

		mask = (1 << id);

		ret = SetThreadAffinityMask(hThread, mask);
	}
#endif

	if (state && ret != FALSE)
		state->cpu_bound_index = id;

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

	if (state && ret == 0)
		state->cpu_bound_index = id;

	return (ret == 0) ? 0 : 1;

#elif defined (TARGET_OS_NETBSD)

	cpuset_t *set;
	int ret;

	set = cpuset_create();
	if (set == NULL)
		return 1;
	
	cpuset_zero(set);
	ret = cpuset_set(id, set);
	if (ret == -1)
		return 1;
	ret = pthread_setaffinity_np(pthread_self(), cpuset_size(set), set);
	cpuset_destroy(set);

	if (state && ret == 0)
		state->cpu_bound_index = id;

	return ret == 0 ? 0 : 1;

#elif defined(TARGET_OS_SOLARIS)

	/*
	 * This requires permissions, so can easily fail.
	 */
	if (processor_bind(P_LWPID, P_MYID, id, NULL) != 0) {
		fprintf(stderr, "warning: failed to bind to CPU%u: %s\n",
			id, strerror(errno));
		return 1;
	}

	if (state)
		state->cpu_bound_index = id;
	return 0;
#elif defined(TARGET_OS_MACOSX)
	int ret = 1;

#ifdef USE_CHUD
	ret = (utilBindThreadToCPU(id) == 0) ? 0 : 1;
#endif

	if (state && ret == 0)
		state->cpu_bound_index = id;

	return ret == 0 ? 0 : 1;
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

/* vim: set ts=4 sts=4 sw=4 noet: */
