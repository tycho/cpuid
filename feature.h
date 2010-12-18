#ifndef __feature_h
#define __feature_h

typedef struct {
	uint8_t stepping:4;
	uint8_t model:4;
	uint8_t family:4;
	uint8_t type:2;
	uint8_t reserved1:2;
	uint8_t extmodel:4;
	uint8_t extfamily:8;
	uint8_t reserved2:4;
} cpu_signature_t;

void print_features(cpu_regs_t *regs, uint32_t level, cpu_vendor_t vendor);

#endif
