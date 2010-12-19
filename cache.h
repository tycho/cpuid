#ifndef __cache_h
#define __cache_h

#include "cpuid.h"
#include "feature.h"

void print_intel_caches(cpu_regs_t *regs, const cpu_signature_t *sig);

#endif
