#ifndef SENI_H
#define SENI_H

#include <stddef.h>
#include "arena.h"
typedef enum {
    ast_int,
    ast_float,
    ast_char,
    ast_double,
    ast_unknown,
} ast_type;

typedef struct {
    char* name;
    ast_type type;
} ast_field;

typedef struct {
    char* name;
    ast_field* fields;
    size_t fields_count;
} ast_struct;

typedef struct {
    ast_struct* structs;
    size_t struct_count;
} ast;

typedef struct {

} diff;

typedef enum {
    PARSE_OUTSIDE,
    PARSE_IN_STRUCT,
    PARSE_READ_FIELD_NAME,
    PARSE_READ_STRUCT_NAME,
} parse_state;

ast parse_header(arena* a,char* header);
diff diff_structs(char* old_header, char* new_header);
#endif