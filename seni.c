#include "seni.h"

#include <string.h>

static int is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

parse_result parse_header(arena* a, char* header) {
    parse_result r = {0};
    r.err = NULL;

    parse_state state = PARSE_OUTSIDE;
    size_t struct_count = 0;
    for (int i = 0; header[i] != '\0'; i++) {
        if (strncmp(&header[i], "typedef struct {", 16) == 0) {
            struct_count++;
        }
    }

    if (struct_count == 0) {
        return r;
    }
    r.value.struct_count = struct_count;
    r.value.structs = allocate(a, sizeof(ast_struct) * struct_count);
    if (!r.value.structs) { r.err = "out of memory"; return r; }

    // count fields per struct
    size_t* field_counts = allocate(a, sizeof(size_t) * struct_count);
    if (!field_counts) { r.err = "out of memory"; return r; }
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

    size_t si = 0;
    size_t fi = 0;
    ast_type cur_type = ast_unknown;

    for (int i = 0; header[i] != '\0'; i++) {
        if (state == PARSE_OUTSIDE && strncmp(&header[i], "typedef struct {", 16) == 0) {
            state = PARSE_IN_STRUCT;
            fi = 0;
            r.value.structs[si].fields = allocate(a, sizeof(ast_field) * field_counts[si]);
            if (!r.value.structs[si].fields) { r.err = "out of memory"; return r; }
            r.value.structs[si].fields_count = 0;
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
            r.value.structs[si].name = arena_copy_string(a, &header[start], len);
            if (!r.value.structs[si].name) { r.err = "out of memory"; return r; }
            r.value.structs[si].fields_count = fi;
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
            } else {
                r.err = "unknown type";
                return r;
            }
            state = PARSE_READ_FIELD_NAME;
        }
        if (state == PARSE_READ_FIELD_NAME && is_white_space(header[i])) {
            continue;
        } else if (state == PARSE_READ_FIELD_NAME) {
            int start = i;
            while (header[i] != ';' && header[i] != ',' && header[i] != '\0') {
                i++;
            }
            if (header[i] == '\0') { r.err = "unexpected end of input"; return r; }
            int len = i - start;
            while (len > 0 && is_white_space(header[start + len - 1])) len--;
            r.value.structs[si].fields[fi].name = arena_copy_string(a, &header[start], len);
            if (!r.value.structs[si].fields[fi].name) { r.err = "out of memory"; return r; }
            r.value.structs[si].fields[fi].type = cur_type;
            fi++;
            if (header[i] == ',') {
                state = PARSE_READ_FIELD_NAME;
            } else {
                state = PARSE_IN_STRUCT;
            }
        }
    }

    return r;
}

diff diff_structs(char *old_header, char *new_header){
    diff d;
    ast old_header_ast;

    return d;
}
