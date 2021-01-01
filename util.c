/*
 * CPUID
 *
 * A simple and small tool to dump/decode CPUID information.
 *
 * Copyright (c) 2010-2021, Steven Noonan <steven@uplinklabs.net>
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

#include "util.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>
#ifdef TARGET_OS_WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#endif

size_t safe_strcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

uint32_t popcnt(uint32_t v)
{
	uint32_t n = 0;
	while (v) {
		n += (v & 1);
		v >>= 1;
	}
	return n;
}

uint32_t count_trailing_zero_bits(uint32_t v)
{
	uint32_t c;
	if (v) {
		v = (v ^ (v - 1)) >> 1; /* Set v's trailing 0s to 1s and zero rest */
		for (c = 0; v; c++)
		{
			v >>= 1;
		}
	} else {
		c = CHAR_BIT * sizeof(v);
	}
	return c;
}

void squeeze(char *str)
{
	int r; /* next character to be read */
	int w; /* next character to be written */

	r=w=0;
	while (str[r])
	{
		if (isspace((int)(str[r])) || iscntrl((int)(str[r])))
		{
			if (w > 0 && !isspace((int)(str[w-1])))
				str[w++] = ' ';
		}
		else
			str[w++] = str[r];
		r++;
	}
	str[w] = 0;
}

double time_sec(void)
{
#ifdef TARGET_OS_WINDOWS
	LARGE_INTEGER freq, ctr;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&ctr);
	return (double)ctr.QuadPart / (double)freq.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
#endif
}

#ifdef TARGET_OS_WINDOWS
int is_windows7_or_greater(void)
{
	static int cached = -1;
	if (cached < 0) {
		OSVERSIONINFOEXW osvi;
		DWORDLONG const dwlConditionMask = VerSetConditionMask(
			VerSetConditionMask(
				VerSetConditionMask(
					0, VER_MAJORVERSION, VER_GREATER_EQUAL),
				VER_MINORVERSION, VER_GREATER_EQUAL),
			VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

		memset(&osvi, 0, sizeof(osvi));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
		osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_WIN7);
		osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_WIN7);
		osvi.wServicePackMajor = 0;

		cached = (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE) ? 1 : 0;
	}
	return cached;
}
#endif

/* vim: set ts=4 sts=4 sw=4 noet: */
