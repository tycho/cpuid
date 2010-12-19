#ifndef __cpuid_h
#define __cpuid_h

typedef struct {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
} cpu_regs_t;

#define ZERO_REGS(x) {memset((x), 0, sizeof(cpu_regs_t));}

BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx);

#include "state.h"

/* Makes a lot of calls easier to do. */
BOOL cpuid_native(cpu_regs_t *regs, cpuid_state_t *state);

const char *reg_to_str(cpu_regs_t *regs);

#endif
