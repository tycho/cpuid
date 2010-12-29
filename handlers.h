#ifndef __handlers_h
#define __handlers_h

typedef void(*cpuid_leaf_handler_t)(struct cpu_regs_t *, struct cpuid_state_t *);

struct cpuid_leaf_handler_index_t {
	uint32_t leaf_id;
	cpuid_leaf_handler_t handler;
};

extern const struct cpuid_leaf_handler_index_t dump_handlers[];
extern const struct cpuid_leaf_handler_index_t decode_handlers[];

#endif
