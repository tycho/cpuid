#include "prefix.h"

#include "cpuid.h"
#include "state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef TARGET_COMPILER_MSVC
#ifdef TARGET_CPU_X86_64
#include <intrin.h>
#endif
#endif

const char *reg_to_str(struct cpu_regs_t *regs)
{
	uint32_t i;
	static char buffer[sizeof(struct cpu_regs_t) + 1];
	buffer[sizeof(struct cpu_regs_t)] = 0;
	memcpy(&buffer, regs, sizeof(struct cpu_regs_t));
	for (i = 0; i < sizeof(struct cpu_regs_t); i++) {
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

BOOL cpuid_native(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	memcpy(&state->last_leaf, regs, sizeof(struct cpu_regs_t));
	return cpuid(&regs->eax, &regs->ebx, &regs->ecx, &regs->edx);
}

BOOL cpuid_load_from_file(const char *filename, struct cpuid_state_t *state)
{
	struct cpuid_leaf_t *leaf;
	size_t linecount = 0;
	FILE *file = fopen(filename, "r");
	if (!file)
		return FALSE;

	while(TRUE) {
		char linebuf[85];

		if(!fgets(linebuf, sizeof(linebuf), file))
			break;

		if (strncmp(linebuf, "CPUID", 5) == 0) {
			linecount++;
		}
	}

	if (linecount < 1)
		goto fail;

	state->cpuid_leaves = (struct cpuid_leaf_t *)malloc(sizeof(struct cpuid_leaf_t) * (linecount + 1));
	assert(state->cpuid_leaves);

	memset(state->cpuid_leaves, 0, sizeof(struct cpuid_leaf_t) * (linecount + 1));

	/* Now do the actual read. */
	rewind(file);

	leaf = state->cpuid_leaves;
	while(TRUE) {
		size_t s;
		char linebuf[85];

		if(!fgets(linebuf, sizeof(linebuf), file))
			break;

		/* Strip \r and \n */
		s = strcspn(linebuf, "\r\n");
		if (s)
			linebuf[s] = 0;

		if (strncmp(linebuf, "CPUID", 5) == 0) {
			/* Probably a valid line. */
			uint32_t eax_in, ecx_in = 0;
			uint32_t eax_out, ebx_out, ecx_out, edx_out;

			/* First format, no ecx input. */
			int r = sscanf(linebuf, "CPUID %08x, results = %08x %08x %08x %08x",
			               &eax_in, &eax_out, &ebx_out, &ecx_out, &edx_out);
			if (r != 5) {
				r = sscanf(linebuf, "CPUID %08x, index %x = %08x %08x %08x %08x",
				           &eax_in, &ecx_in, &eax_out, &ebx_out, &ecx_out, &edx_out);
				if (r != 6) {
					printf("Couldn't parse: '%s'\n", linebuf);
					continue;
				}
			}

			leaf->input.eax = eax_in;
			leaf->input.ecx = ecx_in;

			leaf->output.eax = eax_out;
			leaf->output.ebx = ebx_out;
			leaf->output.ecx = ecx_out;
			leaf->output.edx = edx_out;

			leaf++;
		}
	}

	/* Sentinel values */
	leaf->input.eax = 0xFFFFFFFF;
	leaf->input.ecx = 0xFFFFFFFF;

	leaf->output.eax = 0xFFFFFFFF;
	leaf->output.ebx = 0xFFFFFFFF;
	leaf->output.ecx = 0xFFFFFFFF;
	leaf->output.edx = 0xFFFFFFFF;

	fclose(file);

	return TRUE;
fail:
	if (file)
		fclose(file);

	return FALSE;
}

BOOL cpuid_pseudo(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct cpuid_leaf_t *leaf;

	memcpy(&state->last_leaf, regs, sizeof(struct cpu_regs_t));

	/* Iterate through the loaded leaves and find a match. */
	leaf = state->cpuid_leaves;
	while(leaf && leaf->input.eax != 0xFFFFFFFF) {
		if (leaf->input.eax == regs->eax &&
		    leaf->input.ecx == regs->ecx) {
			memcpy(regs, &leaf->output, sizeof(struct cpu_regs_t));
			return TRUE;
		}
		leaf++;
	}

	/* Didn't find a match. */
	memset(regs, 0, sizeof(struct cpu_regs_t));

	return TRUE;
}

void cpuid_dump_normal(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed)
{
	if (!indexed)
		printf("CPUID %08x, results = %08x %08x %08x %08x | %s\n",
			state->last_leaf.eax,
			regs->eax,
			regs->ebx,
			regs->ecx,
			regs->edx,
			reg_to_str(regs));
	else
		printf("CPUID %08x, index %x = %08x %08x %08x %08x | %s\n",
			state->last_leaf.eax,
			state->last_leaf.ecx,
			regs->eax,
			regs->ebx,
			regs->ecx,
			regs->edx,
			reg_to_str(regs));
}

static const char *uint32_to_binary(uint32_t val)
{
	static char buf[33];
	int i;
	buf[32] = 0;
	for (i = 0; i < 32; i++) {
		buf[31 - i] = (val & (1 << i)) != 0 ? '1' : '0';
	}
	return buf;
}

void cpuid_dump_vmware(struct cpu_regs_t *regs, struct cpuid_state_t *state, unused BOOL indexed)
{
	/* Not sure what VMware's format is for that. */
	if (indexed)
		return;
	printf("cpuid.%x.eax = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(regs->eax));
	printf("cpuid.%x.ebx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(regs->ebx));
	printf("cpuid.%x.ecx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(regs->ecx));
	printf("cpuid.%x.edx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(regs->edx));
}
