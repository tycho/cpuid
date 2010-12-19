#ifndef __prefix_h
#define __prefix_h

#include "platform.h"

#include <stdint.h>
#include <stdlib.h>

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

#ifdef __GNUC__
#define unused __attribute__((unused))
#else
#define unused
#endif

#ifndef BOOL
#define BOOL uint8_t
#endif
#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
