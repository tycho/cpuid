#!/bin/bash
#
# This is a distribution-agnostic build script. Do not use "apt-get", "dnf", or
# similar in here. Add any package installation gunk into the appropriate
# install script instead.
#
set -ex

rm -rf build-{a,ub,t}san build-meson

MESON_ARGS=()
BUILD_VARIANTS=(meson)

BUILD_SANITIZERS=1
[[ $(uname -s) == MINGW* ]] && BUILD_SANITIZERS=0

if lsb_release -c | grep -q bionic$; then
	export ASAN_OPTIONS=detect_odr_violation=0
fi

meson . build-meson -Dwerror=true -Dbuildtype=release -Ddebug=false -Db_lto=true

# Build some tests with sanitizers
if [[ $BUILD_SANITIZERS -ne 0 ]]; then
	BUILD_VARIANTS+=(asan ubsan)
	meson . build-asan -Db_sanitize=address
	meson . build-ubsan -Db_sanitize=undefined
	if [[ ${CXX} == *clang* ]]; then
		BUILD_VARIANTS+=(tsan)
		meson . build-tsan -Db_sanitize=thread
	fi
fi

# Build all targets of meson, ensuring everything can build.
for BUILD_VARIANT in ${BUILD_VARIANTS[@]}; do
	ninja -C build-${BUILD_VARIANT}
done

# Ensure plain makefile build works too
make

# Run basic tests
for BUILD_VARIANT in . ${BUILD_VARIANTS[@]}; do
	BUILD_DIR=build-$BUILD_VARIANT
	[[ "$BUILD_VARIANT" == "." ]] && BUILD_DIR=.
	pushd $BUILD_DIR
		./cpuid -c 0
		./cpuid -c 0 -d
		for DUMP_FORMAT in default etallen sxp vmware xen; do
			./cpuid -c -1 -d --format=$DUMP_FORMAT > dump.txt
			if [[ "$DUMP_FORMAT" == "default" ]] || [[ "$DUMP_FORMAT" == "etallen" ]]; then
				./cpuid -f dump.txt
			fi
		done
	popd
	[[ "$BUILD_VARIANT" != "." ]] && rm -rf build-$BUILD_VARIANT
done
