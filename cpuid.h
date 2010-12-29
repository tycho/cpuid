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

typedef BOOL(*cpuid_call_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *);
typedef void(*cpuid_print_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *, BOOL);

/* Makes a lot of calls easier to do. */
BOOL cpuid_native(struct cpu_regs_t *regs, struct cpuid_state_t *state);
BOOL cpuid_pseudo(struct cpu_regs_t *regs, struct cpuid_state_t *state);

/* Allows printing dumps in different formats. */
void cpuid_dump_normal(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_etallen(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);
void cpuid_dump_vmware(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed);

/* For cpuid_pseudo */
BOOL cpuid_load_from_file(const char *filename, struct cpuid_state_t *state);

const char *reg_to_str(struct cpu_regs_t *regs);

#endif
