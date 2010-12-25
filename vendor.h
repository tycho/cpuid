#ifndef __vendor_h
#define __vendor_h

typedef enum
{
	VENDOR_UNKNOWN   = 0x0,
	VENDOR_INTEL     = 0x1,
	VENDOR_AMD       = 0x2,
	VENDOR_ANY       = 0xFFFFFFFF
} cpu_vendor_t;

typedef enum
{
	HYPERVISOR_UNKNOWN = 0x0,
	HYPERVISOR_XEN  = 0x1,
	HYPERVISOR_VMWARE = 0x2
} hypervisor_t;

#endif
