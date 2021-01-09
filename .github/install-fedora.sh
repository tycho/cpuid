#!/bin/bash
#
# This is an install script for Fedora-specific packages.
#
set -ex

# Base build packages
PACKAGES=(
	git
	gcc
	compiler-rt
	libasan
	libubsan
	clang
	meson
	perl
	pkgconf-pkg-config
	which
)

dnf install -y "${PACKAGES[@]}"
