#ifndef __cache_h
#define __cache_h

struct cpu_regs_t;
struct cpu_signature_t;

void print_intel_caches(struct cpu_regs_t *regs, const struct cpu_signature_t *sig);

#endif
