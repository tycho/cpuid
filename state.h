#ifndef __state_h
#define __state_h

#include "cpuid.h"
#include "vendor.h"

typedef struct {
	uint8_t stepping:4;
	uint8_t model:4;
	uint8_t family:4;
	uint8_t type:2;
	uint8_t reserved1:2;
	uint8_t extmodel:4;
	uint8_t extfamily:8;
	uint8_t reserved2:4;
} cpu_signature_t;

typedef struct
{
	uint32_t stdmax;
	uint32_t extmax;
	uint32_t hvmax;
	cpu_regs_t last_leaf;
	cpu_signature_t sig;
	cpu_vendor_t vendor;
	hypervisor_t hypervisor;
	char procname[48];
} cpuid_state_t;

#define INIT_CPUID_STATE(x) { \
	memset((x), 0, sizeof(cpuid_state_t)); \
	(x)->extmax = 0x80000000; \
	(x)->hvmax = 0x40000000; \
	}

#endif
