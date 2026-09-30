#!/bin/sh

cmake -B build -S .
cmake --build ./build

exec ./build/custom-shell "$@"