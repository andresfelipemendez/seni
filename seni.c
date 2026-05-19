#include "seni.h"
#include "arena.h"

#include <string.h>
#include <stdio.h>

#define MAX_STRUCTS 64
#define STR(x) #x
#define XSTR(x) STR(x)

static int is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

parse_result parse_header(arena* a, char* header) {
    parse_result r = {0};

    // single pass: count structs and fields per struct
    size_t struct_count = 0;
    size_t field_counts[MAX_STRUCTS] = {0};
    int in_struct = 0;
    for (int i = 0; header[i] != '\0'; i++) {
        if (strncmp(&header[i], "typedef struct {", 16) == 0) {
            struct_count++;
            if (struct_count > MAX_STRUCTS) { r.err = "more than " XSTR(MAX_STRUCTS) " structs, too many structs"; return r; }
            in_struct = 1;
            i += 15;
        } else if (in_struct && (header[i] == ',' || header[i] == ';')) {
            field_counts[struct_count - 1]++;
        } else if (in_struct && header[i] == '}') {
            in_struct = 0;
        }
    }

    if (struct_count == 0) {
        return r;
    }
    r.value.struct_count = struct_count;
    r.value.structs = allocate(a, sizeof(ast_struct) * struct_count);
    if (!r.value.structs) { r.err = "out of memory"; return r; }

    parse_state state = PARSE_OUTSIDE;
    size_t si = 0;
    size_t fi = 0;
    ast_type cur_type = ast_unknown;

    int line = 1;
    for (int i = 0; header[i] != '\0'; i++) {
        if (header[i] == '\n') line++;
        if (state == PARSE_OUTSIDE && strncmp(&header[i], "typedef struct {", 16) == 0) {
            state = PARSE_IN_STRUCT;
            fi = 0;
            if (field_counts[si] > 0) {
                r.value.structs[si].fields = allocate(a, sizeof(ast_field) * field_counts[si]);
                if (!r.value.structs[si].fields) { r.err = "out of memory"; return r; }
            }
            r.value.structs[si].fields_count = 0;
            i += 15;
            continue;
        }
        if (state == PARSE_IN_STRUCT && is_white_space(header[i])) {
            continue;
        } else if (state == PARSE_IN_STRUCT && header[i] == '}') {
            i++;
            while (header[i] != '\0' && is_white_space(header[i])) {
                if (header[i] == '\n') line++;
                i++;
            }
            int start = i;
            while (header[i] != ';' && header[i] != '\0') {
                if (header[i] == '\n') line++;
                i++;
            }
            int len = i - start;
            if (len == 0) {
                char* msg = arena_sprintf(a, "struct missing name at line %d", line);
                r.err = msg ? msg : "struct missing name";
                return r;
            }
            r.value.structs[si].name = arena_copy_string(a, &header[start], len);
            if (!r.value.structs[si].name) { r.err = "out of memory"; return r; }
            r.value.structs[si].fields_count = fi;
            si++;
            state = PARSE_OUTSIDE;
            if (header[i] == '\0') break;
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
                int start = i;
                while (header[i] != ' ' && header[i] != '\0') i++;
                int len = i - start;
                char* msg = arena_sprintf(a, "unknown type '%.*s' at line %d", len, &header[start], line);
                r.err = msg ? msg : "unknown type";
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
            if (header[i] == '\0') {
                char* msg = arena_sprintf(a, "unexpected end of input at line %d", line);
                r.err = msg ? msg : "unexpected end of input";
                return r;
            }
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

diff_result diff_structs(arena* a, char *old_header, char *new_header){
    diff_result res = {0};
    parse_result old_header_ast_res = parse_header(a, old_header);
    if (old_header_ast_res.err) {
        char* msg = arena_sprintf(a, "old_header error: %s", old_header_ast_res.err);
        res.err = msg ? msg : old_header_ast_res.err;
        return res;
    }
    ast old_header_ast = old_header_ast_res.value;
    (void)old_header_ast;
    parse_result new_header_ast_res = parse_header(a, new_header);
    if (new_header_ast_res.err) {
      
    }
    return res;
}
