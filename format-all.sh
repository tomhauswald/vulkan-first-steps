#!/bin/bash
clang-format-11 -i $(find -name "*.cc" -or -name "*.h")
