#ifndef __handlers_h
#define __handlers_h

typedef void(*cpuid_leaf_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *);

extern cpuid_leaf_handler_t std_handlers[];
extern cpuid_leaf_handler_t ext_handlers[];
extern cpuid_leaf_handler_t vmm_handlers[];
extern cpuid_leaf_handler_t std_dump_handlers[];
extern cpuid_leaf_handler_t ext_dump_handlers[];
extern cpuid_leaf_handler_t vmm_dump_handlers[];

#define MAX_HANDLER_IDX 0x1F
#define HAS_HANDLER(handlers, ind) ((ind) <= MAX_HANDLER_IDX && (handlers[(ind)]))

#endif
