#!/bin/bash
#
# This is an install script for Arch Linux-specific packages.
#
set -ex

# Base build packages
PACKAGES=(
	base-devel
	ccache
	clang
	meson
	git
	ninja
)

pacman --noconfirm -Syu
pacman --noconfirm -Sy "${PACKAGES[@]}"

exit 0
