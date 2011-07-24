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
