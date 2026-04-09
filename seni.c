#include "seni.h"

#include <string.h>

static int is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

ast parse_header(arena* a, char* header) {
    ast res;
    res.struct_count = 0;
    res.structs = NULL;

    parse_state state;

    state = PARSE_OUTSIDE;
    size_t struct_count = 0;
    for (int i = 0; header[i] != '\0'; i++) {
        if (strncmp(&header[i], "typedef struct {", 16) == 0) {
            struct_count++;
        }
    }

    if (struct_count == 0) {
        return res;
    }
    res.struct_count = struct_count;
    res.structs = allocate(a, sizeof(ast_struct) * struct_count);

    // count fields per struct
    size_t* field_counts = allocate(a, sizeof(size_t) * struct_count);
    for (size_t s = 0; s < struct_count; s++) field_counts[s] = 0;
    int in_struct = 0;
    size_t si_count = 0;
    for (int i = 0; header[i] != '\0'; i++) {
        if (strncmp(&header[i], "typedef struct {", 16) == 0) {
            in_struct = 1;
        } else if (in_struct && (header[i] == ',' || header[i] == ';')) {
            field_counts[si_count]++;
        } else if (in_struct && header[i] == '}') {
            si_count++;
            in_struct = 0;
        }
    }

    size_t si = 0;          // current struct index
    size_t fi = 0;          // current field index within struct
    ast_type cur_type = ast_unknown;

    for (int i = 0; header[i] != '\0'; i++) {
        if (state == PARSE_OUTSIDE && strncmp(&header[i], "typedef struct {", 16) == 0) {
            state = PARSE_IN_STRUCT;
            fi = 0;
            res.structs[si].fields = allocate(a, sizeof(ast_field) * field_counts[si]);
            res.structs[si].fields_count = 0;
            i += 15;
            continue;
        }
        if (state == PARSE_IN_STRUCT && is_white_space(header[i])) {
            continue;
        } else if (state == PARSE_IN_STRUCT && header[i] == '}') {
            i++;
            while (header[i] != '\0' && is_white_space(header[i])) i++;
            int start = i;
            while (header[i] != ';' && header[i] != '\0') {
                i++;
            }
            int len = i - start;
            res.structs[si].name = arena_copy_string(a, &header[start], len);
            res.structs[si].fields_count = fi;
            si++;
            state = PARSE_OUTSIDE;
            continue;
        } else if (state == PARSE_IN_STRUCT) {
            if (strncmp(&header[i], "float ", 6) == 0) {
                cur_type = ast_float;
                i += 5;
            } else if (strncmp(&header[i], "int ", 4) == 0) {
                cur_type = ast_int;
                i += 3;
            } else if (strncmp(&header[i], "char ", 5) == 0) {
                cur_type = ast_char;
                i += 4;
            } else if (strncmp(&header[i], "double ", 7) == 0) {
                cur_type = ast_double;
                i += 6;
            }
            state = PARSE_READ_FIELD_NAME;
        }
        if (state == PARSE_READ_FIELD_NAME && is_white_space(header[i])) {
            continue;
        } else if (state == PARSE_READ_FIELD_NAME) {
            int start = i;
            while (header[i] != ';' && header[i] != ',') {
                i++;
            }
            int len = i - start;
            res.structs[si].fields[fi].name = arena_copy_string(a, &header[start], len);
            res.structs[si].fields[fi].type = cur_type;
            fi++;
            if (header[i] == ',') {
                // same type, next field name (e.g. "float x, y;")
                state = PARSE_READ_FIELD_NAME;
            } else {
                // semicolon — back to reading next type
                state = PARSE_IN_STRUCT;
            }
        }
    }

    return res;
}

diff diff_structs(char *old_header, char *new_header){
    diff d;
    ast old_header_ast;

    return d;
}
