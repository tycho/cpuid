/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2014, Steven Noonan <steven@uplinklabs.net>
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
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#ifdef _MSC_VER
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

extern int ignore_vendor;

#endif
