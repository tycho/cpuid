#include "prefix.h"

#define _USE_GNU
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "cpuid.h"
#include "sanity.h"
#include "threads.h"

typedef int(*sanity_handler_t)();

static int sane_apicid();

sanity_handler_t handlers[] = {
	sane_apicid,
	NULL
};

static unsigned char get_apicid_for_cpu(unsigned int index)
{
	struct cpu_regs_t regs;
	ZERO_REGS(&regs);
	regs.eax = 1;
	thread_bind(index);
	cpuid(&regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
	regs.ebx = regs.ebx >> 24;
	return (char)(regs.ebx & 0xFF);
}

static int apic_compare(const void *a, const void *b)
{
	return (*(const unsigned char *)a - *(const unsigned char *)b);
}

static void *apic_nonsensical_worker_thread(void *flag)
{
	int i, t, s = 0, hwthreads = thread_count();
	while (*(unsigned char *)flag) {
		thread_bind(rand() % hwthreads);
		t = (rand() % 4096 + 1);
		for (i = 0; i < t && *(unsigned char *)flag; i++)
			s = i * t;
		srand(s);
	}
	pthread_exit(NULL);
	return NULL;
}

static int sane_apicid()
{
	unsigned int hwthreads = thread_count(), i, j,
	    worker_count;
	unsigned char *apic_ids = NULL, *apic_copy = NULL,
	    worker_flag;
	pthread_t *workers = NULL;

	printf("Verifying APIC ID sanity... ");

	/* Populate initial APIC ID array. */
	apic_ids = malloc(hwthreads * sizeof(unsigned char));
	for (i = 0; i < hwthreads; i++) {
		apic_ids[i] = get_apicid_for_cpu(i);
	}

	/* First verify that no CPUs reported identical APIC IDs. */
	apic_copy = malloc(hwthreads * sizeof(unsigned char));
	memcpy(apic_copy, apic_ids, hwthreads * sizeof(unsigned char));
	qsort(apic_copy, hwthreads, sizeof(unsigned char), apic_compare);
	for (i = 1; i < hwthreads; i++) {
		if (apic_ids[i - 1] == apic_ids[i]) {
			printf("fail (duplicate APIC IDs)\n");
			return 1;
		}
	}
	free(apic_copy);

	/* Spawn a few busy threads to incur thread migrations,
	   if they're going to happen at all. */
	worker_flag = 1;
	worker_count = hwthreads / 2 + 1;
	workers = malloc(worker_count * sizeof(pthread_t));
	for (i = 0; i < worker_count; i++)
		pthread_create(&workers[i], NULL, apic_nonsensical_worker_thread, (void *)&worker_flag);

	/* Now verify that the APIC IDs don't change over time. */
	for (i = 0; i < 1024; i++) {
		for (j = 0; j < hwthreads; j++) {
			if (apic_ids[j] != get_apicid_for_cpu(j)) {
				printf("fail (APIC IDs changed over time)\n");
				return 2;
			}
		}
	}
	free(apic_ids);

	/* Wait for workers to exit. */
	worker_flag = 0;
	for (i = 0; i < worker_count; i++)
		pthread_join(workers[i], NULL);

	printf("ok\n");
	return 0;
}

int sanity_run()
{
	sanity_handler_t *p = handlers;
	unsigned int ret = 0;
	while (*p) {
		if ((*p++)() != 0)
			ret = p - handlers;
	}
	return ret;
}
