#!/bin/bash

# compile benchmark/

clang -O0 -g -emit-llvm benchmark/pointer.c -c -o pointer.bc
# clang -O3 -emit-llvm benchmark/hello.c -c -o hello.bc
