#ifndef __threads_h
#define __threads_h

struct cpuid_state_t;

typedef int (*thread_bind_handler_t)(struct cpuid_state_t *, uint32_t);
typedef uint32_t (*thread_count_handler_t)(struct cpuid_state_t *);

/* These do real thread binding. */
int thread_bind_native(struct cpuid_state_t *state, uint32_t id);
uint32_t thread_count_native(struct cpuid_state_t *state);

/* These are used to change the logical selector in the state structure. */
int thread_bind_stub(struct cpuid_state_t *state, uint32_t id);
uint32_t thread_count_stub(struct cpuid_state_t *state);

/* These aren't used during CPUID runs, just in sanity check runs. */
unsigned int thread_get_binding(void);
unsigned int thread_bind_mask(unsigned int mask);

#endif
