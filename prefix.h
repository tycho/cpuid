#ifndef __prefix_h
#define __prefix_h

#include "platform.h"

#ifdef TARGET_OS_LINUX
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef _MSC_VER
/* MSVC doesn't actually support most of C99, but we define
   this so that the below actually uses the 'inline' keyword,
   which MSVC does understand. */
#define C99
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

#ifndef __unused
#ifdef __GNUC__
#define __unused __attribute__((unused))
#else
#define __unused
#endif
#endif

#ifndef TARGET_OS_WINDOWS
#ifndef BOOL
#define BOOL uint8_t
#endif
#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#else
#include <windows.h>
#endif

extern int ignore_vendor;

#endif
