#!/bin/bash
#
# This is a post-install script common to all Docker images.
#
set -x
meson --version
g++ --version
clang++ --version
lsb_release -a
exit 0
