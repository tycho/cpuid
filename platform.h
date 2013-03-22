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

#ifndef __platform_h
#define __platform_h

#undef PROCESSOR_DETECTED
#undef COMPILER_DETECTED
#undef OS_DETECTED

/* ------------------- *
* PROCESSOR DETECTION *
* ------------------- */

/* Carbon defines this for us on Mac, apparently... */
#if defined (TARGET_CPU_PPC)
#define PROCESSOR_DETECTED
#endif

/* ARM */
#if !defined (PROCESSOR_DETECTED)
#if defined (__arm__)
#define PROCESSOR_DETECTED
#define TARGET_CPU_ARM
#define TARGET_LITTLE_ENDIAN
#endif
#endif

/* DEC Alpha */
#if !defined (PROCESSOR_DETECTED)
#if defined (__alpha) || defined (__alpha__)
#define PROCESSOR_DETECTED
#define TARGET_CPU_ALPHA
#define TARGET_LITTLE_ENDIAN /* How should bi-endianness be handled? */
#endif
#endif

/* Sun SPARC */
#if !defined (PROCESSOR_DETECTED)
#if defined (__sparc) || defined (__sparc__)
#define PROCESSOR_DETECTED
#define TARGET_CPU_SPARC
#define TARGET_BIG_ENDIAN
#endif
#endif

/* PowerPC */
#if !defined (PROCESSOR_DETECTED)
#if defined (_ARCH_PPC) || defined (__ppc__) || defined (__ppc64__) || defined (__PPC) || defined (powerpc) || defined (__PPC__) || defined (__powerpc64__) || defined (__powerpc64)
#define PROCESSOR_DETECTED
#if defined (__ppc64__) || defined (__powerpc64__) || defined (__powerpc64)
#define TARGET_CPU_PPC 64
#else
#define TARGET_CPU_PPC 32
#endif
#define TARGET_BIG_ENDIAN
#endif
#endif

/* x86_64 or AMD64 or x64 */
#if !defined (PROCESSOR_DETECTED)
#if defined (__x86_64__) || defined (__x86_64) || defined (__amd64) || defined (__amd64__) || defined (_AMD64_) || defined (_M_X64)
#define PROCESSOR_DETECTED
#define TARGET_CPU_X64
#define TARGET_CPU_X86_64
#define TARGET_LITTLE_ENDIAN
#endif
#endif

/* Intel x86 */
#if !defined (PROCESSOR_DETECTED)
#if defined (__i386__) || defined (__i386) || defined (i386) || defined (_X86_) || defined (_M_IX86)
#define PROCESSOR_DETECTED
#define TARGET_CPU_X86
#define TARGET_LITTLE_ENDIAN
#endif
#endif

/* IA64 */
#if !defined (PROCESSOR_DETECTED)
#if defined (__ia64__) || defined (_IA64) || defined (__ia64) || defined (_M_IA64)
#define PROCESSOR_DETECTED
#define TARGET_CPU_IA64
#define TARGET_LITTLE_ENDIAN
#endif
#endif

/* ------------------- *
* COMPILER DETECTION  *
* ------------------- */

#if !defined (COMPILER_DETECTED)
#if defined (__GNUC__)
#define COMPILER_DETECTED
#define TARGET_COMPILER_GCC
#endif
#if defined (__CYGWIN__) || defined (__CYGWIN32__)
#define TARGET_COMPILER_CYGWIN
#define TARGET_OS_WINDOWS
#define OS_DETECTED
#endif
#if defined (__MINGW32__)
#define TARGET_COMPILER_MINGW
#endif
#endif

#if !defined (COMPILER_DETECTED)
#if defined (__INTEL_COMPILER) || defined (__ICL)
#define COMPILER_DETECTED
#define TARGET_COMPILER_ICC
#endif
#endif

#if !defined (COMPILER_DETECTED)
#if defined (_MSC_VER)
#define COMPILER_DETECTED
#define TARGET_COMPILER_MSVC
#endif
#endif

#if !defined (COMPILER_DETECTED)
#if defined (__BORLANDC__)
#define COMPILER_DETECTED
#define TARGET_COMPILER_BORLAND
#endif
#endif

/* ------------ *
* OS DETECTION *
* ------------ */

#if !defined (OS_DETECTED)
#if defined (TARGET_COMPILER_VC) || defined (_WIN32) || defined (_WIN64)
#define OS_DETECTED
#define TARGET_OS_WINDOWS
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__linux__) || defined (linux) || defined (__linux) || defined (__gnu_linux__) || defined (__CYGWIN__)
#define OS_DETECTED
#define TARGET_OS_LINUX
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (TARGET_CPU_ARM)
#define OS_DETECTED
#define TARGET_OS_NDSFIRMWARE
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__HAIKU__)
#define OS_DETECTED
#define TARGET_OS_HAIKU
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__FreeBSD__)
#define OS_DETECTED
#define TARGET_OS_FREEBSD
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__NetBSD__)
#define OS_DETECTED
#define TARGET_OS_NETBSD
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__OpenBSD__)
#define OS_DETECTED
#define TARGET_OS_OPENBSD
#endif
#endif

#if !defined (OS_DETECTED)
#if defined (__APPLE__) || defined (__MACH__)
#define OS_DETECTED
#define TARGET_OS_MACOSX
#endif
#endif

#if defined (_LP64) || defined (__LP64__) || defined (_M_X64) || defined(_M_IA64)
#define TARGET_CPU_BITS 64
#else
#define TARGET_CPU_BITS 32
#endif

#if defined (TARGET_CPU_PPC)
#if TARGET_CPU_PPC == 32
#define TARGET_CPU_BITS 32
#elif TARGET_CPU_PPC == 64
#define TARGET_CPU_BITS 64
#endif
#endif

#if !defined (TARGET_CPU_BITS)
#define TARGET_CPU_BITS 0
#endif

#endif
