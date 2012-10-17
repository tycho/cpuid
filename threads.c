#include "prefix.h"

#include <stdio.h>

#ifdef TARGET_OS_LINUX

#include <pthread.h>
#include <sched.h>
#define CPUSET_T cpu_set_t

#elif defined(TARGET_OS_WINDOWS)

#include <windows.h>

#elif defined(TARGET_OS_FREEBSD)

#include <pthread.h>
#include <pthread_np.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#define CPUSET_T cpuset_t

#elif defined(TARGET_OS_MACOSX)

#include <sys/sysctl.h>

//#define USE_CHUD
#ifdef USE_CHUD
extern int chudProcessorCount(void);
extern int utilBindThreadToCPU(int n);
extern int utilUnbindThreadFromCPU(void);
#endif

#endif

#include "state.h"
#include "threads.h"

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
	for (i = 0; i < 32; i++) {
		if (thread_bind_native(state, i) != 0)
			break;
	}
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
#ifdef TARGET_OS_MACOSX
#ifdef USE_CHUD
	return (utilBindThreadToCPU(id) == 0) ? 0 : 1;
#else
	return 1;
#endif
#else
	return thread_bind_mask(1 << id);
#endif
}

int thread_bind_stub(struct cpuid_state_t *state, uint32_t id)
{
	assert(state);
	assert(id < state->cpu_logical_count);
	state->cpu_bound_index = id;
	return 0;
}
