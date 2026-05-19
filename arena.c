#include "arena.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

char* arena_copy_string(arena* a, const char* src, size_t len) {
    char* dst = allocate(a, len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

char* arena_sprintf(arena* a, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    char* buf = allocate(a, n);
    if (!buf) return NULL;
    va_start(args, fmt);
    vsnprintf(buf, n, fmt, args);
    va_end(args);
    return buf;
}
