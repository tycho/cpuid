#include "prefix.h"
#include "cpuid.h"

#include <string.h>

const char *reg_to_str(cpu_regs_t *regs)
{
	uint32_t i;
	static char buffer[sizeof(cpu_regs_t) + 1];
	buffer[sizeof(cpu_regs_t)] = 0;
	memcpy(&buffer, regs, sizeof(cpu_regs_t));
	for (i = 0; i < sizeof(cpu_regs_t); i++) {
		if (buffer[i]  > 31 && buffer[i] < 127)
			continue;
		buffer[i] = '.';
	}
	return buffer;
}

#ifdef TARGET_COMPILER_MSVC
BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
	/*
	 *
	 * VERY broken version for MSVC right now.
	 *
	 */*
#if 0
#ifdef TARGET_CPU_X86
	/* This is pretty much the standard way to detect whether the CPUID
	 *     instruction is supported: try to change the ID bit in the EFLAGS
	 *     register.  If we can change it, then the CPUID instruction is
	 *     implemented.  */

	uint32_t pre_change, post_change;
	const uint32_t id_flag = 0x200000;

	__asm {
		mov edx, id_flag;
		pushfd;                         /* Save %eflags to restore later.  */
		pushfd;                         /* Push second copy, for manipulation.  */
		pop ebx;                        /* Pop it into post_change.  */
		mov eax, ebx;                   /* Save copy in pre_change.   */
		xor ebx, edx;                   /* Tweak bit in post_change.  */
		push ebx;                       /* Push tweaked copy... */
		popfd;                          /* ... and pop it into eflags.  */
		pushfd;                         /* Did it change?  Push new %eflags... */
		pop ebx;                        /* ... and pop it into post_change.  */
		popfd;                          /* Restore original value.  */
		mov pre_change, eax;
		mov post_change, ebx;
	}

	if ((pre_change ^ post_change) & id_flag) == 0)
		return FALSE;
#endif

	__asm {
		push esi;
		push edi;
		mov eax, request;
		cpuid;
		mov edi, [_eax];
		mov esi, [_ebx];
		mov[edi], eax;
		mov[esi], ebx;
		mov edi, [_ecx];
		mov esi, [_edx];
		mov[edi], ecx;
		mov[esi], edx;
		pop edi;
		pop esi;
	}

	return TRUE;
#endif
}
#endif

#ifdef TARGET_COMPILER_GCC
BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
#ifdef TARGET_CPU_X86
	uint32_t pre_change, post_change;
	const uint32_t id_flag = 0x200000;
	asm ("pushfl\n\t"          /* Save %eflags to restore later.  */
		 "pushfl\n\t"          /* Push second copy, for manipulation.  */
		 "popl %1\n\t"         /* Pop it into post_change.  */
		 "movl %1,%0\n\t"      /* Save copy in pre_change.   */
		 "xorl %2,%1\n\t"      /* Tweak bit in post_change.  */
		 "pushl %1\n\t"        /* Push tweaked copy... */
		 "popfl\n\t"           /* ... and pop it into %eflags.  */
		 "pushfl\n\t"          /* Did it change?  Push new %eflags... */
		 "popl %1\n\t"         /* ... and pop it into post_change.  */
		 "popfl"               /* Restore original value.  */
		 : "=&r" (pre_change), "=&r" (post_change)
		 : "ir" (id_flag));
	if ((pre_change ^ post_change) & id_flag) == 0)
		return FALSE;
#endif
	asm volatile("cpuid"
	    : "=a" (*_eax),
	      "=b" (*_ebx),
	      "=c" (*_ecx),
	      "=d" (*_edx)
	    : "0" (*_eax), "2" (*_ecx));
	return TRUE;
}

#endif

BOOL cpuid_native(cpu_regs_t *regs, cpuid_state_t *state)
{
	memcpy(&state->last_leaf, regs, sizeof(cpu_regs_t));
	return cpuid(&regs->eax, &regs->ebx, &regs->ecx, &regs->edx);
}
