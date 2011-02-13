#include "prefix.h"

#include <stdio.h>

#ifdef TARGET_OS_LINUX

#include <pthread.h>
#include <sched.h>
#define CPUSET_T cpu_set_t

#elif defined(TARGET_OS_WINDOWS)

#include <windows.h>

#elif defined(TARGET_OS_FREEBSD)

#include <pthread_np.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#define CPUSET_T cpuset_t

#elif defined(TARGET_OS_MACOSX)

extern int chudProcessorCount();
extern int utilBindThreadToCPU(int n);
extern int utilUnbindThreadFromCPU();

#endif

#include "threads.h"

unsigned int thread_count()
{
	static unsigned int i = 0;
	unsigned long long min = 0, max = (unsigned int)-1;
	if (i) return i;
	if (thread_bind(0) != 0) {
		fprintf(stderr, "ERROR: thread_bind() doesn't appear to be working correctly.\n");
		abort();
	}
	while(max - min > 0) {
		i = (max + min) >> 1LL;
		if (thread_bind(i) == 0) {
			min = i + 1;
		} else {
			max = i;
		}
	}
	i = min;
	return i;
}

unsigned int thread_bind(unsigned int id)
{
#ifdef TARGET_OS_WINDOWS

	DWORD ret;
	HANDLE hThread = GetCurrentThread();
	ret = SetThreadAffinityMask(hThread, 1 << id);

	return (ret != 0) ? 0 : 1;

#elif defined(TARGET_OS_LINUX) || defined(TARGET_OS_FREEBSD)

	int ret;
	CPUSET_T mask;
	pthread_t pth;

	pth = pthread_self();
	CPU_ZERO(&mask);
	CPU_SET(id, &mask);
	ret = pthread_setaffinity_np(pth, sizeof(mask), &mask);

	return (ret == 0) ? 0 : 1;

#elif defined(TARGET_OS_MACOSX)

	return (utilBindThreadToCPU(id) == 0) ? 0 : 1;

#else
#error "thread_bind() not defined for this platform"
#endif
}
