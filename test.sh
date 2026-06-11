#!/bin/sh
set -e
gcc test.c -o test.out
./test.out
gcc test_e2e.c -o test_e2e.out
./test_e2e.out
