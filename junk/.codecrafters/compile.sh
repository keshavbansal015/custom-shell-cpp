#!/bin/sh
#
# This script is used to compile your program on CodeCrafters
#
# This runs before .codecrafters/run.sh
#
# Learn more: https://codecrafters.io/program-interface

set -e # Exit on failure

# Initialize vcpkg if needed
if [ ! -d "$HOME/vcpkg" ]; then
    git clone https://github.com/microsoft/vcpkg $HOME/vcpkg
fi

# Build the project using vcpkg toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build ./build
