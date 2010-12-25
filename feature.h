#ifndef __feature_h
#define __feature_h

#include "cpuid.h"
#include "vendor.h"
#include "state.h"

void print_features(cpu_regs_t *regs, cpuid_state_t *state);

#endif
