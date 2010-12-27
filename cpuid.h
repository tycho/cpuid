#ifndef __cpuid_h
#define __cpuid_h

struct cpu_regs_t{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

#define ZERO_REGS(x) {memset((x), 0, sizeof(struct cpu_regs_t));}

BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx);

struct cpuid_state_t;

/* Makes a lot of calls easier to do. */
BOOL cpuid_native(struct cpu_regs_t *regs, struct cpuid_state_t *state);

const char *reg_to_str(struct cpu_regs_t *regs);

#endif
