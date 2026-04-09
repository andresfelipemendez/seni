#ifndef ARENA_H
#define ARENA_H
#include "stdio.h"
typedef struct {
    void* data;
    size_t size;
    size_t offset;
} arena;

static inline void create_arena(arena* a, void* buf, size_t size) {
    a->data = buf;
    a->size = size;
    a->offset = 0;
}

static inline void* allocate(arena* a, size_t s) {
    if (a->offset + s > a->size) return NULL;
    void* ptr = (char*)a->data + a->offset;
    a->offset += s;
    return ptr;
}

char* arena_copy_string(arena* a, const char* src, size_t len);
#endif