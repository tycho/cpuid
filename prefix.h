/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2020, Steven Noonan <steven@uplinklabs.net>
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

#ifndef __prefix_h
#define __prefix_h

#include "platform.h"

#ifdef TARGET_OS_LINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#ifdef TARGET_OS_WINDOWS
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#ifdef TARGET_COMPILER_VC
/* MSVC doesn't actually support most of C99, but we define
   this so that the below actually uses the 'inline' keyword,
   which MSVC does understand. */
#define C99

#define NOMINMAX

#if _MSC_VER <= 1200
#pragma warning (error: 4013)
#pragma warning (disable: 4761)
#endif

#endif

#if defined(__STDC__)
# define C89
# if defined(__STDC_VERSION__)
#  define C90
#  if (__STDC_VERSION__ >= 199409L)
#   define C94
#  endif
#  if (__STDC_VERSION__ >= 199901L)
#   define C99
#  endif
# endif
#endif

#ifndef C99
#define inline
#endif

#ifndef __unused_variable
#ifdef __GNUC__
#define __unused_variable __attribute__((unused))
#else
#define __unused_variable
#endif
#endif

#ifndef TARGET_OS_WINDOWS
#ifndef BOOL
#define BOOL uint8_t
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#else
#include <windows.h>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#include <stdio.h>

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

static inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

static inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}
#endif

extern int ignore_vendor;

#endif

/* vim: set ts=4 sts=4 sw=4 noet: */
