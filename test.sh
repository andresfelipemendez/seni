#!/bin/sh
set -e
# library must be strict c89; test harness uses utest.h (default std)
gcc -std=c89 -pedantic -Wall -Werror -fsyntax-only seni.c arena.c platform_linux.c
gcc test.c -o test.out
./test.out
gcc test_e2e.c -o test_e2e.out
./test_e2e.out
