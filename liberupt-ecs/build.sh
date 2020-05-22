#!/bin/bash
clang++-11 -std=c++20 -O3 -S -masm=intel *.cc
clang++-11 -std=c++20 -O3 *.cc -o ecs-test.elf
