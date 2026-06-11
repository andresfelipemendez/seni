#include "platform.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int platform_make_dir(const char* path) {
    if (CreateDirectoryA(path, NULL)) return 0;
    return GetLastError() == ERROR_ALREADY_EXISTS ? 0 : 1;
}

const char* platform_lib_extension(void) {
    return "dll";
}

int platform_compile_shared(const char* src_path, const char* lib_path, const char* err_path) {
    char cmd[1024];
    sprintf(cmd, "gcc -std=c89 -pedantic -shared -o %s %s 2> %s", lib_path, src_path, err_path);
    return system(cmd);
}

platform_lib platform_load_lib(const char* path) {
    HMODULE m = LoadLibraryA(path);
    if (!m) fprintf(stderr, "LoadLibrary failed for %s (error %lu)\n", path, GetLastError());
    return (platform_lib)m;
}

void* platform_get_symbol(platform_lib lib, const char* name) {
    /* c89 forbids function-pointer -> object-pointer casts; launder through
       an integer (size_t is pointer-sized on win32/win64) */
    return (void*)(size_t)GetProcAddress((HMODULE)lib, name);
}

void platform_unload_lib(platform_lib lib) {
    if (lib) FreeLibrary((HMODULE)lib);
}
