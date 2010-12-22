#ifndef __state_h
#define __state_h

#include "cpuid.h"
#include "feature.h"
#include "vendor.h"

typedef struct
{
	uint32_t stdmax;
	uint32_t extmax;
	cpu_regs_t last_leaf;
	cpu_signature_t sig;
	cpu_vendor_t vendor;
	char procname[48];
} cpuid_state_t;

#define INIT_CPUID_STATE(x) { memset((x), 0, sizeof(cpuid_state_t)); (x)->extmax = 0x80000000; }

#endif
