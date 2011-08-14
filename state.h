#ifndef __state_h
#define __state_h

#include "cpuid.h"
#include "threads.h"
#include "vendor.h"

struct cpu_signature_t {
	unsigned stepping:4;
	unsigned model:4;
	unsigned family:4;
	unsigned type:2;
	unsigned reserved1:2;
	unsigned extmodel:4;
	unsigned extfamily:8;
	unsigned reserved2:4;
};

struct cpuid_leaf_t {
	struct cpu_regs_t input;
	struct cpu_regs_t output;
};

struct cpuid_state_t
{
	thread_bind_handler_t thread_bind;
	thread_count_handler_t thread_count;
	cpuid_call_handler_t cpuid_call;
	cpuid_print_handler_t cpuid_print;

	/* These two are stubs for thread_bind/thread_count. */
	uint32_t cpu_bound_index;
	uint32_t cpu_logical_count;

	struct cpuid_leaf_t **cpuid_leaves;
	struct cpu_regs_t last_leaf;
	struct cpu_signature_t sig;
	cpu_vendor_t vendor;
	hypervisor_t hypervisor;
	uint32_t curmax;
	char procname[48];
};

#define INIT_CPUID_STATE(x) { \
	memset((x), 0, sizeof(struct cpuid_state_t)); \
	(x)->cpuid_print = cpuid_dump_normal; \
	(x)->cpuid_call = cpuid_native; \
	(x)->thread_bind = thread_bind_native; \
	(x)->thread_count = thread_count_native; \
	}

#define FREE_CPUID_STATE(x) { \
	if ((x)->cpuid_leaves) \
		free((x)->cpuid_leaves); \
	}

#endif
