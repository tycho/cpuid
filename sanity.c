#include "prefix.h"

#define _USE_GNU
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "cpuid.h"
#include "sanity.h"
#include "state.h"
#include "threads.h"

typedef int(*sanity_handler_t)(struct cpuid_state_t *state);

static int sane_apicid(struct cpuid_state_t *state);
static int sane_l3_sharing(struct cpuid_state_t *state);

sanity_handler_t handlers[] = {
	sane_apicid,
	sane_l3_sharing,
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
	thread_bind(index);
	return get_apicid(state);
}

static void *apic_nonsensical_worker_thread(void *flagptr)
{
	uint8_t *flag = (uint8_t *)flagptr;
	uint32_t hwthreads = thread_count();
	struct timeval tv;
	while (*flag) {
		thread_bind(rand() % hwthreads);
		gettimeofday(&tv, NULL);
	}
	pthread_exit(NULL);
	return NULL;
}

struct apic_validate_t {
	struct cpuid_state_t *state;
	uint32_t index;
	uint8_t *worker_flag;
	uint8_t expected;
	uint8_t failed;
};

static void *apic_validation_thread(void *ptr)
{
	struct apic_validate_t *meta = (struct apic_validate_t *)ptr;
	thread_bind(meta->index);
	while (!meta->failed && *meta->worker_flag) {
		usleep(5000);
		if (get_apicid(meta->state) != meta->expected) {
			meta->failed = 1;
		}
	}
	pthread_exit(NULL);
	return NULL;
}

static int apic_compare(const void *a, const void *b)
{
	return (*(const uint8_t *)a - *(const uint8_t *)b);
}

static int sane_apicid(struct cpuid_state_t *state)
{
	int ret = 0;
	uint32_t hwthreads = thread_count(), i, c,
	    worker_count, oldbinding;
	uint8_t *apic_ids = NULL, *apic_copy = NULL, worker_flag;
	struct apic_validate_t *apic_state = NULL;
	struct timeval start, now;
	pthread_t *busy_workers = NULL,
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

	/* Spawn a few busy threads to incur thread migrations,
	   if they're going to happen at all. */
	worker_flag = 1;
	busy_workers = malloc(worker_count * sizeof(pthread_t));
	for (i = 0; i < worker_count; i++)
		pthread_create(&busy_workers[i], NULL, apic_nonsensical_worker_thread, &worker_flag);

	/* Now verify that the APIC IDs don't change over time. */
	apic_state = malloc(hwthreads * sizeof(struct apic_validate_t));
	apic_workers = malloc(hwthreads * sizeof(pthread_t));
	memset(apic_state, 0, hwthreads * sizeof(struct apic_validate_t));
	for (i = 0; i < hwthreads; i++) {
		apic_state[i].state = state;
		apic_state[i].index = i;
		apic_state[i].expected = apic_ids[i];
		apic_state[i].worker_flag = &worker_flag;
		pthread_create(&apic_workers[i], NULL, apic_validation_thread, &apic_state[i]);
	}
	free(apic_ids);

	/* Occasionally signal workers to run validation checks. */
	gettimeofday(&start, NULL);
	c = 1;
	printf(".");
	fflush(stdout);
	while(worker_flag) {
		gettimeofday(&now, NULL);
		if (now.tv_sec - start.tv_sec > 30)
			break;
		if (c % 100 == 0) {
			c = 1;
			printf(".");
			fflush(stdout);
		} else {
			c++;
		}
		usleep(10000);
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
		for (i = 0; i < worker_count; i++) {
			pthread_join(busy_workers[i], NULL);
		}
		free(busy_workers);
	}

	if (apic_workers) {
		for (i = 0; i < hwthreads; i++) {
			pthread_join(apic_workers[i], NULL);
		}
		free(apic_workers);
	}
	
	if (apic_state)
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

int sanity_run(struct cpuid_state_t *state)
{
	sanity_handler_t *p = handlers;
	unsigned int ret = 0;
	while (*p) {
		if ((*p++)(state) != 0)
			ret = p - handlers;
	}
	return ret;
}
