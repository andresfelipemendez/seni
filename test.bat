@echo off
gcc test.c -o test.exe || exit /b 1
.\test.exe || exit /b 1
gcc test_e2e.c -o test_e2e.exe || exit /b 1
.\test_e2e.exe
