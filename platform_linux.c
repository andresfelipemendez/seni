#include "platform.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int platform_make_dir(const char* path) {
    if (mkdir(path, 0755) == 0) return 0;
    return errno == EEXIST ? 0 : 1;
}

const char* platform_lib_extension(void) {
    return "so";
}

int platform_compile_shared(const char* src_path, const char* lib_path, const char* err_path) {
    char cmd[1024];
    sprintf(cmd, "gcc -std=c89 -pedantic -shared -fPIC -o %s %s 2> %s", lib_path, src_path, err_path);
    return system(cmd);
}

platform_lib platform_load_lib(const char* path) {
    /* "./" prefix: dlopen searches LD_LIBRARY_PATH for bare names, not the cwd */
    char full[512];
    void* m;
    sprintf(full, "./%s", path);
    m = dlopen(full, RTLD_NOW);
    if (!m) fprintf(stderr, "dlopen failed for %s: %s\n", path, dlerror());
    return m;
}

void* platform_get_symbol(platform_lib lib, const char* name) {
    return dlsym(lib, name);
}

void platform_unload_lib(platform_lib lib) {
    if (lib) dlclose(lib);
}
