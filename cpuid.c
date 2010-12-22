#include "prefix.h"
#include "cpuid.h"

#include <string.h>
#ifdef TARGET_COMPILER_MSVC
#ifdef TARGET_CPU_X86_64
#include <intrin.h>
#endif
#endif

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

#if defined(TARGET_COMPILER_MSVC)
#ifdef TARGET_CPU_X86_64

/* I hate this. */
BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
	uint32_t regs[4];
	__cpuidex(regs, *_eax, *_ecx);
	*_eax = regs[0];
	*_ebx = regs[1];
	*_ecx = regs[2];
	*_edx = regs[3];
	return TRUE;
}

#else

/* MSVC, x86-only. Stupid compiler doesn't allow __asm on x86_64. */
BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
#ifdef TARGET_CPU_X86
	static BOOL cpuid_support = FALSE;

	if (!cpuid_support) {
		uint32_t pre_change, post_change;
		const uint32_t id_flag = 0x200000;

		/* This is pretty much the standard way to detect whether the CPUID
		 *     instruction is supported: try to change the ID bit in the EFLAGS
		 *     register.  If we can change it, then the CPUID instruction is
		 *     implemented.  */
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

		if (((pre_change ^ post_change) & id_flag) == 0)
			return FALSE;
		else
			cpuid_support = TRUE;
	}
#endif

	__asm {
		mov esi, _eax;
		mov edi, _ecx;
		mov eax, DWORD PTR [esi];
		mov ecx, DWORD PTR [edi];
		cpuid;
		mov DWORD PTR [esi], eax;
		mov DWORD PTR [edi], ecx;
		mov esi, _ebx;
		mov edi, _edx;
		mov DWORD PTR [esi], ebx;
		mov DWORD PTR [edi], edx;
	}

	return TRUE;
}

#endif
#endif

#ifdef TARGET_COMPILER_GCC
BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
#ifdef TARGET_CPU_X86
	static BOOL cpuid_support = FALSE;
	if (!cpuid_support) {
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
		if (((pre_change ^ post_change) & id_flag) == 0)
			return FALSE;
		else
			cpuid_support = TRUE;
	}
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
