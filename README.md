# 遷移 Seni

inspired by sql migration this is a library to generate memory migration code for hotreloading dlls it will diff previous structs and new ones, from there generate the code to migrate the existing memory to the new layout when hot-reloading

the "old" header comes from the running dll itself: `seni_embed.h` embeds the
header bytes into the dll at build time (assembler `.incbin`, no codegen step)
and exports them as `const char* seni_layout`. at reload time the engine reads
that symbol from the currently-loaded dll, diffs it against the new header on
disk, generates + compiles the migration, runs it, then swaps dlls. this can't
desync — the layout travels inside the binary it describes.

using utest for testing
https://github.com/sheredom/utest.h

## tests

`test.bat` (windows) or `test.sh` (linux, e.g. `wsl sh test.sh`) runs unit
tests (test.c) then end-to-end tests (test_e2e.c).
The e2e tests read header pairs from `fixtures/`, generate migration code,
compile it with gcc into a shared library in `build/` (dll on windows, so on
linux), load it, run the migration on a real memory block and assert the
resulting layout. Platform specifics (mkdir, compile command, dynamic
loading) live behind `platform.h`, implemented by `platform_windows.c` and
`platform_linux.c`.