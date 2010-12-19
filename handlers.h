#ifndef __handlers_h
#define __handlers_h

#include "cpuid.h"
#include "state.h"

typedef void(*cpu_std_handler)(cpu_regs_t *, cpuid_state_t *);

extern cpu_std_handler std_handlers[];
extern cpu_std_handler ext_handlers[];
extern cpu_std_handler std_dump_handlers[];
extern cpu_std_handler ext_dump_handlers[];

#define MAX_HANDLER_IDX 0x0F
#define HAS_HANDLER(handlers, ind) ((ind) <= MAX_HANDLER_IDX && (handlers[(ind)]))

#endif
