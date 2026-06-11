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

typedef enum { field_op_copy, field_op_zero } field_op_kind;

typedef struct {
    field_op_kind kind;
    char* name;        // field name in new struct
    ast_type type;     // type in new struct
} field_op;

typedef struct {
    char* name;                                 // struct name
    ast_field* old_fields; size_t old_count;    // for emitting old typedef
    ast_field* new_fields; size_t new_count;    // for emitting new typedef
    field_op* ops;         size_t ops_count;    // one per new field, in new order
} struct_diff;

typedef struct {
    struct_diff* structs;
    size_t struct_count;
} diff;

typedef enum {
    PARSE_OUTSIDE,
    PARSE_IN_STRUCT,
    PARSE_READ_FIELD_NAME,
    PARSE_READ_STRUCT_NAME,
} parse_state;

typedef struct {
    ast value;
    char* err;  // NULL = success
} parse_result;

typedef struct {
    diff value;
    char* err;  // NULL = success
} diff_result;

typedef struct {
    char* code;
    char* err;  // NULL = success
} generate_result;

parse_result parse_header(arena* a, char* header);
diff_result diff_structs(arena* a, char* old_header, char* new_header);
generate_result generate_migration(arena* a, diff d);
#endif
