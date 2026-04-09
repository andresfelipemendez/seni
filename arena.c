#include "arena.h"
#include <string.h>

char* arena_copy_string(arena* a, const char* src, size_t len) {
    char* dst = allocate(a, len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}
