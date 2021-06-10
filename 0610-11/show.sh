#!/bin/sh
llvm-as matrix.ll -o a.bc
llc a.bc -o a.s
gcc -c a.s -o a.o
clang++ a.o -o a
rm a.bc a.o a.s
