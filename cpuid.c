/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2013, Steven Noonan <steven@uplinklabs.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "prefix.h"

#include "state.h"

#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#ifdef TARGET_COMPILER_MSVC
#ifdef TARGET_CPU_X86_64
#include <intrin.h>
#endif
#endif

static const char *reg_to_str(char *buffer, struct cpu_regs_t *regs)
{
	uint32_t i;
	buffer[sizeof(struct cpu_regs_t)] = 0;
	memcpy(buffer, regs, sizeof(struct cpu_regs_t));
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
	asm volatile(
	    "cpuid"
	    : "=a" (*_eax),
	      "=b" (*_ebx),
	      "=c" (*_ecx),
	      "=d" (*_edx)
	    : "0" (*_eax), "2" (*_ecx));
	return TRUE;
}

#endif

#ifdef __linux__
BOOL cpuid_kernel(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	off64_t offset = ((uint64_t)regs->ecx << sizeof(uint32_t)) | (uint64_t)regs->eax;
	char path[32];
	BOOL ret = FALSE;
	static int fd = -1;
	static unsigned int i;
	int r;

	if (fd == -1 || i != state->cpu_bound_index) {
		i = state->cpu_bound_index;

		if (fd != -1)
			close(fd);

		sprintf(path, "/dev/cpu/%u/cpuid", i);

		fd = open(path, O_LARGEFILE, O_RDONLY);
	}

	if (fd == -1)
		goto out;

	memset(path, 0, sizeof(path));
	memcpy(&state->last_leaf, regs, sizeof(struct cpu_regs_t));

	if (offset != lseek(fd, offset, SEEK_SET))
		goto cleanup;

	r = read(fd, regs, 16);
	if (r == -1)
		goto cleanup;

	ret = TRUE;
cleanup:
	close(fd);
	i = -1;
	fd = -1;
out:
	return ret;
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
	size_t i, cpucount, leafcount, leafcount_tmp;
	FILE *file = fopen(filename, "r");

	if (!file)
		return FALSE;

	cpucount = 0;
	leafcount = 0;
	leafcount_tmp = 0;
	while(TRUE) {
		char linebuf[128];

		if(!fgets(linebuf, sizeof(linebuf), file))
			break;

		/* CPU %d:\n */
		if (strncmp(linebuf, "CPU ", 4) == 0) {
			uint32_t id;
			sscanf(linebuf, "CPU %u:", &id);
			cpucount = (cpucount > id + 1) ? cpucount : id + 1;
			leafcount = (leafcount > leafcount_tmp) ? leafcount : leafcount_tmp;
			leafcount_tmp = 0;
		}

		/* CPUID %08x:%02x [...] */
		if (strncmp(linebuf, "CPUID", 5) == 0 ||
			strncmp(linebuf, "   0x", 4) == 0) {
			leafcount_tmp++;
		}
	}
	leafcount = (leafcount > leafcount_tmp) ? leafcount : leafcount_tmp;

	if (leafcount < 1)
		goto fail;

	/* Compatibility with old dumps with only one CPU. */
	if (cpucount < 1)
		cpucount = 1;

	state->cpu_logical_count = cpucount;

	state->cpuid_leaves = (struct cpuid_leaf_t **)malloc(sizeof(struct cpuid_leaf_t *) * (cpucount + 1));
	assert(state->cpuid_leaves);
	for (i = 0; i < cpucount; i++) {
		state->cpuid_leaves[i] = (struct cpuid_leaf_t *)malloc(sizeof(struct cpuid_leaf_t) * (leafcount + 1));
		assert(state->cpuid_leaves[i]);
		memset(state->cpuid_leaves[i], 0xFF, sizeof(struct cpuid_leaf_t) * (leafcount + 1));
	}

	state->cpuid_leaves[cpucount] = NULL;

	/* Now do the actual read. */
	rewind(file);

	leaf = NULL;
	while(TRUE) {
		size_t s;
		char linebuf[128];

		if(!fgets(linebuf, sizeof(linebuf), file))
			break;

		/* Strip \r and \n */
		s = strcspn(linebuf, "\r\n");
		if (s)
			linebuf[s] = 0;

		if (strncmp(linebuf, "CPU ", 4) == 0) {
			uint32_t id;
			int r;

			r = sscanf(linebuf, "CPU %u:", &id);
			assert(r == 1);

			/* We already have a leaf. Finalize it. */
			if (leaf) {
				leaf->input.eax = 0xFFFFFFFF;
				leaf->input.ecx = 0xFFFFFFFF;

				leaf->output.eax = 0xFFFFFFFF;
				leaf->output.ebx = 0xFFFFFFFF;
				leaf->output.ecx = 0xFFFFFFFF;
				leaf->output.edx = 0xFFFFFFFF;
			}

			assert(id < state->cpu_logical_count);
			leaf = state->cpuid_leaves[id];
		}

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
					r = sscanf(linebuf, "CPUID %08x:%02x = %08x %08x %08x %08x",
							   &eax_in, &ecx_in, &eax_out, &ebx_out, &ecx_out, &edx_out);
					if (r != 6)
						continue;
				}
			}

			if (!leaf) {
				/* No 'CPU %u:' header, assumed CPU 0 */
				leaf = state->cpuid_leaves[0];
			}
			assert(leaf);
			leaf->input.eax = eax_in;
			leaf->input.ecx = ecx_in;

			leaf->output.eax = eax_out;
			leaf->output.ebx = ebx_out;
			leaf->output.ecx = ecx_out;
			leaf->output.edx = edx_out;

			leaf++;
		} else  if (strncmp(linebuf, "   0x", 4) == 0) {
			/* Probably a valid line. */
			uint32_t eax_in, ecx_in = 0;
			uint32_t eax_out, ebx_out, ecx_out, edx_out;

			/* First format, no ecx input. */
			int r = sscanf(linebuf, "   0x%08x 0x%02x: eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x",
			               &eax_in, &ecx_in, &eax_out, &ebx_out, &ecx_out, &edx_out);

			if (r != 6)
				continue;

			if (!leaf) {
				/* No 'CPU %u:' header, assumed CPU 0 */
				leaf = state->cpuid_leaves[0];
			}
			assert(leaf);
			leaf->input.eax = eax_in;
			leaf->input.ecx = ecx_in;

			leaf->output.eax = eax_out;
			leaf->output.ebx = ebx_out;
			leaf->output.ecx = ecx_out;
			leaf->output.edx = edx_out;

			leaf++;
		}
	}

	/* Sentinel values
	 *
	 * It may seem strange to populate these again, but it's possible
	 * to have a file without 'CPU %u:' lines, so we only have CPU 0.
	 * This also means we never hit the sentinel population above.
	 */
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

BOOL cpuid_stub(struct cpu_regs_t *regs, struct cpuid_state_t *state)
{
	struct cpuid_leaf_t *leaf;

	memcpy(&state->last_leaf, regs, sizeof(struct cpu_regs_t));

	/* Iterate through the loaded leaves and find a match. */
	leaf = state->cpuid_leaves[state->cpu_bound_index];
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

static const char *uint32_to_binary(char *buffer, uint32_t val)
{
	int i;
	buffer[32] = 0;
	for (i = 0; i < 32; i++) {
		buffer[31 - i] = (val & (1 << i)) != 0 ? '1' : '0';
	}
	return buffer;
}

void cpuid_dump_normal(struct cpu_regs_t *regs, struct cpuid_state_t *state, __unused_variable BOOL indexed)
{
	char buffer[sizeof(struct cpu_regs_t) + 1];
	printf("CPUID %08x:%02x = %08x %08x %08x %08x | %s\n",
	       state->last_leaf.eax,
	       state->last_leaf.ecx,
	       regs->eax,
	       regs->ebx,
	       regs->ecx,
	       regs->edx,
	       reg_to_str(buffer, regs));
}

void cpuid_dump_xen(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed)
{
	char eax[33],
	     ebx[33],
	     ecx[33],
	     edx[33];

	/* Skip the hypervisor leaf. */
	if ((0xF0000000 & state->last_leaf.eax) == 0x40000000)
		return;

	uint32_to_binary(eax, regs->eax);
	uint32_to_binary(ebx, regs->ebx);
	uint32_to_binary(ecx, regs->ecx);
	uint32_to_binary(edx, regs->edx);

	if (!indexed)
		printf("    '0x%08x:eax=%s,ebx=%s,ecx=%s,edx=%s',\n",
		       state->last_leaf.eax,
		       eax, ebx, ecx, edx);
	else
		printf("    '0x%08x,%d:eax=%s,ebx=%s,ecx=%s,edx=%s',\n",
		       state->last_leaf.eax,
		       state->last_leaf.ecx,
		       eax, ebx, ecx, edx);
}

void cpuid_dump_xen_sxp(struct cpu_regs_t *regs, struct cpuid_state_t *state, BOOL indexed)
{
	char eax[33],
	     ebx[33],
	     ecx[33],
	     edx[33];

	/* Skip the hypervisor leaf. */
	if ((0xF0000000 & state->last_leaf.eax) == 0x40000000)
		return;

	uint32_to_binary(eax, regs->eax);
	uint32_to_binary(ebx, regs->ebx);
	uint32_to_binary(ecx, regs->ecx);
	uint32_to_binary(edx, regs->edx);

	if (!indexed)
		printf("(0x%08x   ((eax %s)\n"
			   "               (ebx %s)\n"
			   "               (ecx %s)\n"
			   "               (edx %s)))\n",
		       state->last_leaf.eax,
		       eax, ebx, ecx, edx);
	else
		printf("(0x%08x,%d ((eax %s)\n"
			   "               (ebx %s)\n"
			   "               (ecx %s)\n"
			   "               (edx %s)))\n",
		       state->last_leaf.eax,
		       state->last_leaf.ecx,
		       eax, ebx, ecx, edx);
}


void cpuid_dump_etallen(struct cpu_regs_t *regs, struct cpuid_state_t *state, __unused_variable BOOL indexed)
{
	printf("   0x%08x 0x%02x: eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x\n",
	       state->last_leaf.eax,
	       state->last_leaf.ecx,
	       regs->eax,
	       regs->ebx,
	       regs->ecx,
	       regs->edx);
}

void cpuid_dump_vmware(struct cpu_regs_t *regs, struct cpuid_state_t *state, __unused_variable BOOL indexed)
{
	char buffer[33];
	/* Not sure what VMware's format is for that. */
	if (indexed)
		return;
	/* Skip the hypervisor leaf. */
	if ((0xF0000000 & state->last_leaf.eax) == 0x40000000)
		return;
	printf("cpuid.%x.eax = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(buffer, regs->eax));
	printf("cpuid.%x.ebx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(buffer, regs->ebx));
	printf("cpuid.%x.ecx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(buffer, regs->ecx));
	printf("cpuid.%x.edx = \"%s\"\n", state->last_leaf.eax, uint32_to_binary(buffer, regs->edx));
}
