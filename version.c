#include "prefix.h"

#include "build.h"
#include "license.h"
#include "version.h"

#include <stdio.h>

void license()
{
	puts(CPUID_LICENSE);
}

const char *cpuid_version_short()
{
	return CPUID_VERSION_TAG;
}

const char *cpuid_version_long()
{
	return CPUID_VERSION_LONG;
}

int cpuid_version_major()
{
	return CPUID_VERSION_MAJOR;
}

int cpuid_version_minor()
{
	return CPUID_VERSION_MINOR;
}

int cpuid_version_revison()
{
	return CPUID_VERSION_REVISION;
}

int cpuid_version_build()
{
	return CPUID_VERSION_BUILD;
}