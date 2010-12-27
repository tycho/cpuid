#ifndef __feature_h
#define __feature_h

struct cpu_regs_t;
struct cpuid_state_t;

void print_features(struct cpu_regs_t *regs, struct cpuid_state_t *state);

#endif
