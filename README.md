# 遷移 Seni

inspired by sql migration this is a library to generate memory migration code for hotreloading dlls it will diff previous structs and new ones, from there generate the code to migrate the existing memory to the new layout when hot-reloading

using utest for testing
https://github.com/sheredom/utest.h

## tests

`test.bat` runs unit tests (test.c) then end-to-end tests (test_e2e.c).
The e2e tests read header pairs from `fixtures/`, generate migration code,
compile it with gcc into a DLL in `build/`, load it, run the migration on a
real memory block and assert the resulting layout.