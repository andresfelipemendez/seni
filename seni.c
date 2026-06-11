#include "seni.h"
#include "arena.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX_STRUCTS 64
#define STR(x) #x
#define XSTR(x) STR(x)

static int is_white_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

parse_result parse_header(arena* a, char* header) {
    parse_result r = {0};
    size_t struct_count = 0;
    size_t field_counts[MAX_STRUCTS] = {0};
    int in_struct = 0;
    int i;
    parse_state state;
    size_t si;
    size_t fi;
    ast_type cur_type;
    int line;

    /* single pass: count structs and fields per struct */
    for (i = 0; header[i] != '\0'; i++) {
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

    state = PARSE_OUTSIDE;
    si = 0;
    fi = 0;
    cur_type = ast_unknown;

    line = 1;
    for (i = 0; header[i] != '\0'; i++) {
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
            int start;
            int len;
            i++;
            while (header[i] != '\0' && is_white_space(header[i])) {
                if (header[i] == '\n') line++;
                i++;
            }
            start = i;
            while (header[i] != ';' && header[i] != '\0') {
                if (header[i] == '\n') line++;
                i++;
            }
            len = i - start;
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
                int len;
                char* msg;
                while (header[i] != ' ' && header[i] != '\0') i++;
                len = i - start;
                msg = arena_sprintf(a, "unknown type '%.*s' at line %d", len, &header[start], line);
                r.err = msg ? msg : "unknown type";
                return r;
            }
            state = PARSE_READ_FIELD_NAME;
        }
        if (state == PARSE_READ_FIELD_NAME && is_white_space(header[i])) {
            continue;
        } else if (state == PARSE_READ_FIELD_NAME) {
            int start = i;
            int len;
            while (header[i] != ';' && header[i] != ',' && header[i] != '\0') {
                i++;
            }
            if (header[i] == '\0') {
                char* msg = arena_sprintf(a, "unexpected end of input at line %d", line);
                r.err = msg ? msg : "unexpected end of input";
                return r;
            }
            len = i - start;
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

static const char* type_name(ast_type t) {
    switch (t) {
        case ast_int: return "int";
        case ast_float: return "float";
        case ast_char: return "char";
        case ast_double: return "double";
        default: return "void";
    }
}

/* arena-backed string builder: consecutive appends are contiguous because the
   arena hands out sequential memory; each append backs off its own null
   terminator so the next append overwrites it.
   c89: no vsnprintf, so format into a fixed temp first. */
typedef struct {
    arena* a;
    char* start;
    char* err;
} strbuf;

static void sb_appendf(strbuf* b, const char* fmt, ...) {
    char tmp[1024];
    va_list args;
    int n;
    char* dst;
    if (b->err) return;
    va_start(args, fmt);
    n = vsprintf(tmp, fmt, args);
    va_end(args);
    if (n < 0) { b->err = "format error"; return; }
    dst = allocate(b->a, (size_t)n + 1);
    if (!dst) { b->err = "out of memory"; return; }
    if (!b->start) b->start = dst;
    memcpy(dst, tmp, (size_t)n + 1);
    b->a->offset -= 1;
}

generate_result generate_migration(arena* a, diff d) {
    generate_result r = {0};
    strbuf b;
    size_t i;
    b.a = a;
    b.start = NULL;
    b.err = NULL;
    sb_appendf(&b, "#include <stddef.h>\n\n"
                   "#if defined(_WIN32)\n"
                   "#define SENI_EXPORT __declspec(dllexport)\n"
                   "#else\n"
                   "#define SENI_EXPORT\n"
                   "#endif\n\n");
    for (i = 0; i < d.struct_count; i++) {
        struct_diff* sd = &d.structs[i];
        size_t j;
        if (sd->old_count > 0) {
            sb_appendf(&b, "typedef struct { ");
            for (j = 0; j < sd->old_count; j++)
                sb_appendf(&b, "%s %s; ", type_name(sd->old_fields[j].type), sd->old_fields[j].name);
            sb_appendf(&b, "} %s_old;\n", sd->name);
        }
        sb_appendf(&b, "typedef struct { ");
        for (j = 0; j < sd->new_count; j++)
            sb_appendf(&b, "%s %s; ", type_name(sd->new_fields[j].type), sd->new_fields[j].name);
        sb_appendf(&b, "} %s_new;\n", sd->name);

        sb_appendf(&b, "SENI_EXPORT void migrate_%s(void* old_p, void* new_p, size_t count) {\n", sd->name);
        if (sd->old_count > 0)
            sb_appendf(&b, "    %s_old* o = (%s_old*)old_p;\n", sd->name, sd->name);
        sb_appendf(&b, "    %s_new* n = (%s_new*)new_p;\n", sd->name, sd->name);
        sb_appendf(&b, "    size_t i;\n");
        if (sd->old_count == 0)
            sb_appendf(&b, "    (void)old_p;\n");
        sb_appendf(&b, "    for (i = 0; i < count; i++) {\n");
        for (j = 0; j < sd->ops_count; j++) {
            field_op* op = &sd->ops[j];
            if (op->kind == field_op_copy)
                sb_appendf(&b, "        n[i].%s = o[i].%s;\n", op->name, op->name);
            else
                sb_appendf(&b, "        n[i].%s = 0;\n", op->name);
        }
        sb_appendf(&b, "    }\n}\n\n");
    }
    if (b.err) { r.err = b.err; return r; }
    allocate(a, 1); /* claim the final null terminator so later allocations don't clobber it */
    r.code = b.start;
    return r;
}

diff_result diff_structs(arena* a, char *old_header, char *new_header){
    diff_result res = {0};
    parse_result old_r;
    parse_result new_r;
    ast old_ast;
    ast new_ast;
    size_t i;

    old_r = parse_header(a, old_header);
    if (old_r.err) {
        char* msg = arena_sprintf(a, "old_header error: %s", old_r.err);
        res.err = msg ? msg : old_r.err;
        return res;
    }
    new_r = parse_header(a, new_header);
    if (new_r.err) {
        char* msg = arena_sprintf(a, "new_header error: %s", new_r.err);
        res.err = msg ? msg : new_r.err;
        return res;
    }
    old_ast = old_r.value;
    new_ast = new_r.value;

    res.value.struct_count = new_ast.struct_count;
    if (new_ast.struct_count == 0) return res;
    res.value.structs = allocate(a, sizeof(struct_diff) * new_ast.struct_count);
    if (!res.value.structs) { res.err = "out of memory"; return res; }

    for (i = 0; i < new_ast.struct_count; i++) {
        ast_struct* ns = &new_ast.structs[i];
        struct_diff* sd = &res.value.structs[i];
        ast_struct* os = NULL;
        size_t j;
        size_t f;

        sd->name = ns->name;
        sd->new_fields = ns->fields;
        sd->new_count = ns->fields_count;

        for (j = 0; j < old_ast.struct_count; j++) {
            if (strcmp(old_ast.structs[j].name, ns->name) == 0) { os = &old_ast.structs[j]; break; }
        }
        sd->old_fields = os ? os->fields : NULL;
        sd->old_count = os ? os->fields_count : 0;

        sd->ops_count = ns->fields_count;
        sd->ops = NULL;
        if (ns->fields_count > 0) {
            sd->ops = allocate(a, sizeof(field_op) * ns->fields_count);
            if (!sd->ops) { res.err = "out of memory"; return res; }
        }
        for (f = 0; f < ns->fields_count; f++) {
            field_op* op = &sd->ops[f];
            size_t g;
            op->name = ns->fields[f].name;
            op->type = ns->fields[f].type;
            op->kind = field_op_zero;
            for (g = 0; os && g < os->fields_count; g++) {
                if (strcmp(os->fields[g].name, ns->fields[f].name) == 0) { op->kind = field_op_copy; break; }
            }
        }
    }
    return res;
}
