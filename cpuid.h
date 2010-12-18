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

static inline BOOL cpuid_native(cpu_regs_t *regs) {
	return cpuid(&regs->eax, &regs->ebx, &regs->ecx, &regs->edx);
};

const char *reg_to_str(cpu_regs_t *regs);

#endif
