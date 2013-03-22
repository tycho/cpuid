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

#include "build.h"
#include "license.h"

#include "version.h"

#include <stdio.h>

void license(void)
{
	puts(CPUID_LICENSE);
}

const char *cpuid_version_short(void)
{
	return CPUID_VERSION_TAG;
}

const char *cpuid_version_long(void)
{
	return CPUID_VERSION_LONG;
}

int cpuid_version_major(void)
{
	return CPUID_VERSION_MAJOR;
}

int cpuid_version_minor(void)
{
	return CPUID_VERSION_MINOR;
}

int cpuid_version_revison(void)
{
	return CPUID_VERSION_REVISION;
}

int cpuid_version_build(void)
{
	return CPUID_VERSION_BUILD;
}
