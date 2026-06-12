/* c89 + -pedantic hides realpath and PATH_MAX behind feature macros;
   200809L is the POSIX revision that adopted realpath */
#define _POSIX_C_SOURCE 200809L

#include "platform.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int platform_make_dir(const char* path) {
    if (mkdir(path, 0755) == 0) return 0;
    return errno == EEXIST ? 0 : 1;
}

const char* platform_lib_extension(void) {
    return "so";
}

int platform_absolute_path(const char* rel_path, char* out, size_t cap) {
    char full[PATH_MAX];
    size_t n;
    if (!realpath(rel_path, full)) {
        fprintf(stderr, "realpath failed for %s: %s\n", rel_path, strerror(errno));
        return 1;
    }
    n = strlen(full);
    if (n + 1 > cap) {
        fprintf(stderr, "absolute path for %s is %lu bytes, cap %lu\n",
                rel_path, (unsigned long)n + 1, (unsigned long)cap);
        return 1;
    }
    memcpy(out, full, n + 1);
    return 0;
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
