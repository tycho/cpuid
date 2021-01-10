#!/bin/bash
#
# This is an install script for Alpine-specific packages.
#
set -ex

apk update

# Base build packages
PACKAGES=(
	clang
	clang-dev
	llvm-dev
	gcc
	g++
	ccache
	make
	meson
	ninja
	pkgconf
	git
	perl
	linux-headers
)

apk add "${PACKAGES[@]}"

exit 0
