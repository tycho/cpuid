#ifndef __handlers_h
#define __handlers_h

#include "cpuid.h"
#include "state.h"

typedef void(*cpu_std_handler)(cpu_regs_t *, cpuid_state_t *);

void handle_features(cpu_regs_t *regs, cpuid_state_t *state);

void handle_std_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_cache(cpu_regs_t *regs, cpuid_state_t *state);
void handle_std_x2apic(cpu_regs_t *regs, cpuid_state_t *state);

void handle_ext_base(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_pname(cpu_regs_t *regs, cpuid_state_t *state);
void handle_ext_l2cachefeat(cpu_regs_t *regs, cpuid_state_t *state);

void handle_dump_std_04(cpu_regs_t *regs, cpuid_state_t *state);
void handle_dump_std_0B(cpu_regs_t *regs, cpuid_state_t *state);

extern cpu_std_handler std_handlers[];
extern cpu_std_handler ext_handlers[];
extern cpu_std_handler std_dump_handlers[];
extern cpu_std_handler ext_dump_handlers[];

#define MAX_HANDLER_IDX 0x0F
#define HAS_HANDLER(handlers, ind) ((ind) <= MAX_HANDLER_IDX && (handlers[(ind)]))

#endif
