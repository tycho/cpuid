/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2014, Steven Noonan <steven@uplinklabs.net>
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

#ifdef TARGET_OS_WINDOWS
#include <windows.h>
#else
#define _USE_GNU
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <string.h>

#include "clock.h"
#include "sanity.h"
#include "state.h"
#include "util.h"

typedef int(*sanity_handler_t)(struct cpuid_state_t *state);

static int sane_apicid(struct cpuid_state_t *state);
static int sane_l3_sharing(struct cpuid_state_t *state);
static int sane_performance(struct cpuid_state_t *state);

sanity_handler_t handlers[] = {
	sane_apicid,
	sane_l3_sharing,
	sane_performance,
	NULL
};

static uint8_t get_apicid(struct cpuid_state_t *state)
{
	struct cpu_regs_t regs;
	ZERO_REGS(&regs);
	regs.eax = 1;
	state->cpuid_call(&regs, state);
	regs.ebx = regs.ebx >> 24;
	return (char)(regs.ebx & 0xFF);
}

static uint8_t get_apicid_for_cpu(struct cpuid_state_t *state, uint32_t index)
{
	state->thread_bind(state, index);
	return get_apicid(state);
}

#ifdef TARGET_OS_WINDOWS
static DWORD WINAPI apic_nonsensical_worker_thread(LPVOID flagptr)
{
	uint8_t *flag = (uint8_t *)flagptr;
	uint32_t hwthreads = thread_count_native(NULL);
	while (*flag) {
		SetThreadAffinityMask(GetCurrentThread(), 1 << (rand() % hwthreads));
		Sleep(1);
	}
	return 0;
}
#else
static void *apic_nonsensical_worker_thread(void *flagptr)
{
	uint8_t *flag = (uint8_t *)flagptr;
	uint32_t hwthreads = thread_count_native(NULL);
	struct timeval tv;
	while (*flag) {
		thread_bind_native(NULL, rand() % hwthreads);
		gettimeofday(&tv, NULL);
	}
	pthread_exit(NULL);
	return NULL;
}
#endif

struct apic_validate_t {
	struct cpuid_state_t *state;
	uint32_t index;
	uint8_t *worker_flag;
	uint8_t expected;
	uint8_t failed;
};

#ifdef TARGET_OS_WINDOWS
static DWORD WINAPI apic_validation_thread(LPVOID ptr)
{
	struct apic_validate_t *meta = (struct apic_validate_t *)ptr;
	SetThreadAffinityMask(GetCurrentThread(), 1 << meta->index);
	while (!meta->failed && *meta->worker_flag) {
		Sleep(5);
		if (get_apicid(meta->state) != meta->expected) {
			meta->failed = 1;
		}
	}
	return 0;
}
#else
static void *apic_validation_thread(void *ptr)
{
	struct apic_validate_t *meta = (struct apic_validate_t *)ptr;
	thread_bind_native(NULL, meta->index);
	while (!meta->failed && *meta->worker_flag) {
		usleep(5000);
		if (get_apicid(meta->state) != meta->expected) {
			meta->failed = 1;
		}
	}
	pthread_exit(NULL);
	return NULL;
}
#endif

static int apic_compare(const void *a, const void *b)
{
	return (*(const uint8_t *)a - *(const uint8_t *)b);
}

#ifdef TARGET_OS_WINDOWS
typedef HANDLE thread_handle_t;
#else
typedef pthread_t thread_handle_t;
#endif

static int sane_apicid(struct cpuid_state_t *state)
{
	int ret = 0;
	uint32_t hwthreads = thread_count_native(NULL), i, c,
	         worker_count, oldbinding;
	uint8_t *apic_ids = NULL, *apic_copy = NULL, worker_flag;
	struct apic_validate_t *apic_state = NULL;
	double start, now;
	thread_handle_t *busy_workers = NULL,
	                *apic_workers = NULL;

	worker_count = hwthreads / 4 + 1;

	/* Store the current affinity mask. It will get clobbered. */
	oldbinding = thread_get_binding();

	printf("Verifying APIC ID sanity");

	/* Populate initial APIC ID array. */
	apic_ids = malloc(hwthreads * sizeof(unsigned char));
	for (i = 0; i < hwthreads; i++) {
		apic_ids[i] = get_apicid_for_cpu(state, i);
	}

	/* First verify that no CPUs reported identical APIC IDs. */
	apic_copy = malloc(hwthreads * sizeof(unsigned char));
	memcpy(apic_copy, apic_ids, hwthreads * sizeof(unsigned char));
	qsort(apic_copy, hwthreads, sizeof(unsigned char), apic_compare);
	for (i = 1; i < hwthreads; i++) {
		if (apic_ids[i - 1] == apic_ids[i]) {
			printf("fail (duplicate APIC IDs)\n");
			ret = 1;
			goto cleanup;
		}
	}
	free(apic_copy);
	apic_copy = NULL;

	/* Spawn a few busy threads to incur thread migrations,
	   if they're going to happen at all. */
	worker_flag = 1;
	busy_workers = malloc(worker_count * sizeof(thread_handle_t));
	for (i = 0; i < worker_count; i++)
#ifdef TARGET_OS_WINDOWS
		busy_workers[i] = CreateThread(NULL, 0, apic_nonsensical_worker_thread, &worker_flag, 0, NULL);
#else
		pthread_create(&busy_workers[i], NULL, apic_nonsensical_worker_thread, &worker_flag);
#endif

	/* Now verify that the APIC IDs don't change over time. */
	apic_state = malloc(hwthreads * sizeof(struct apic_validate_t));
	apic_workers = malloc(hwthreads * sizeof(thread_handle_t));
	memset(apic_state, 0, hwthreads * sizeof(struct apic_validate_t));
	for (i = 0; i < hwthreads; i++) {
		apic_state[i].state = state;
		apic_state[i].index = i;
		apic_state[i].expected = apic_ids[i];
		apic_state[i].worker_flag = &worker_flag;
#ifdef TARGET_OS_WINDOWS
		apic_workers[i] = CreateThread(NULL, 0, apic_validation_thread, &apic_state[i], 0, NULL);
#else
		pthread_create(&apic_workers[i], NULL, apic_validation_thread, &apic_state[i]);
#endif
	}
	free(apic_ids);
	apic_ids = NULL;

	/* Occasionally signal workers to run validation checks. */
	start = time_sec();
	c = 1;
	printf(".");
	fflush(stdout);
	while(worker_flag) {
		now = time_sec();
		if (now - start > 30.0)
			break;
		if (c % 100 == 0) {
			c = 1;
			printf(".");
			fflush(stdout);
		} else {
			c++;
		}
#ifdef TARGET_OS_WINDOWS
		Sleep(10);
#else
		usleep(10000);
#endif
		for (i = 0; i < hwthreads; i++) {
			if (apic_state[i].failed) {
				printf(" failed (APIC IDs changed over time)\n");
				ret = 2;
				goto cleanup;
			}
		}
	}

	printf(" ok\n");

cleanup:
	/* Wait for workers to exit. */
	worker_flag = 0;
	if (busy_workers) {
#ifdef TARGET_OS_WINDOWS
		WaitForMultipleObjects(worker_count, busy_workers, TRUE, INFINITE);
		for (i = 0; i < worker_count; i++)
			CloseHandle(busy_workers[i]);
#else
		for (i = 0; i < worker_count; i++) {
			pthread_join(busy_workers[i], NULL);
		}
#endif
		free(busy_workers);
	}

	if (apic_workers) {
#ifdef TARGET_OS_WINDOWS
		WaitForMultipleObjects(hwthreads, apic_workers, TRUE, INFINITE);
		for (i = 0; i < hwthreads; i++)
			CloseHandle(apic_workers[i]);
#else
		for (i = 0; i < hwthreads; i++) {
			pthread_join(apic_workers[i], NULL);
		}
#endif
		free(apic_workers);
	}
	
	free(apic_ids);
	free(apic_copy);
	free(apic_state);

	/* Restore the affinity mask from before. */
	thread_bind_mask(oldbinding);

	return ret;
}

static int sane_l3_sharing(struct cpuid_state_t *state)
{
	struct eax_cache04_t {
		unsigned type:5;
		unsigned level:3;
		unsigned self_initializing:1;
		unsigned fully_associative:1;
		unsigned reserved:4;
		unsigned max_threads_sharing:12; /* +1 encoded */
		unsigned apics_reserved:6; /* +1 encoded */
	};
	uint32_t i = 0;
	struct cpu_regs_t regs;
	printf("Verifying L3 thread sharing sanity... ");
	while (1) {
		struct eax_cache04_t *eax;
		ZERO_REGS(&regs);
		regs.eax = 4;
		regs.ecx = i;
		state->cpuid_call(&regs, state);

		if (!(regs.eax & 0xF))
			break;

		eax = (struct eax_cache04_t *)&regs.eax;

		if (eax->level == 3 && eax->max_threads_sharing + 1 == 1) {
			printf("fail (L3 cache shared by too few threads)\n");
			return 0;
		}

		i++;
	}
	printf("ok\n");
	return 0;
}

static int measure_leaf(struct cpuid_state_t *state, uint32_t eax, uint32_t ecx)
{
	uint64_t s, e;
	uint32_t i = 0, max = 500000;
	struct cpu_regs_t regs;

	printf("Measuring performance of leaf 0x%08x:%d... ", eax, ecx);
	s = get_cpu_clock();
	while (i < max) {
		ZERO_REGS(&regs);
		regs.eax = eax;
		regs.ecx = ecx;
		state->cpuid_call(&regs, state);

		i++;
	}
	e = get_cpu_clock();

	//printf("\ns: %" PRIu64 "\ne: %" PRIu64 "\n", s, e);
	printf("total: %" PRIu64 " ns (%" PRIu64 " clocks), ", cpu_clock_to_wall(e - s), e - s);
	printf("per call: %" PRIu64 " ns (%" PRIu64 " clocks)\n", cpu_clock_to_wall(e - s) / max, (e - s) / max);
	return 0;
}

static int sane_performance(struct cpuid_state_t *state)
{
	struct leaf {
		uint32_t eax;
		uint32_t ecx;
	} leaves[] = {
		{ 0x00000000, 0 },
		{ 0x40000000, 0 },
		{ 0x80000000, 0 }
	};
	size_t leaf_count = sizeof(leaves) / sizeof(leaves[0]);
	size_t i;
	for (i = 0; i < leaf_count; i++)
		measure_leaf(state, leaves[i].eax, leaves[i].ecx);
	return 0;
}

int sanity_run(struct cpuid_state_t *state)
{
	sanity_handler_t *p = handlers;
	unsigned int ret = 0;
#ifdef TARGET_OS_WINDOWS
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	timeBeginPeriod(tc.wPeriodMin);
#endif
	while (*p) {
		if ((*p++)(state) != 0)
			ret = p - handlers;
	}
#ifdef TARGET_OS_WINDOWS
	timeEndPeriod(tc.wPeriodMin);
#endif
	return ret;
}
