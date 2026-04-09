#include "utest.h"
#include "seni.h"
#include "arena.h"
#include "arena.c"
#include "seni.c"

UTEST_MAIN()
UTEST(parse, header) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "float x, y;"
    "} enemy;";
    parse_result r = parse_header(&a, header);
    ASSERT_FALSE(r.err);
    ASSERT_EQ((size_t)1,r.value.struct_count);
    ASSERT_EQ((size_t)2,r.value.structs[0].fields_count);
    ASSERT_STREQ("enemy",r.value.structs[0].name);
    ASSERT_STREQ("x",r.value.structs[0].fields[0].name);
    ASSERT_EQ(ast_float,r.value.structs[0].fields[0].type);
}

UTEST(parse, unknown_type) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {\n"
        "unsigned int x;\n"
    "} enemy;";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("unknown type 'unsigned' at line 2", r.err);
}

UTEST(parse, missing_semicolon) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {\n"
        "int x";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("unexpected end of input at line 2", r.err);
}

UTEST(parse, out_of_memory) {
    char buf[32];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "float x, y, z;"
        "int a, b, c;"
        "double d, e, f;"
    "} big;";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("out of memory", r.err);
}

UTEST(parse, trailing_spaces_in_names) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "float x , y ;"
    "} enemy;";
    parse_result r = parse_header(&a, header);
    ASSERT_FALSE(r.err);
    ASSERT_STREQ("x", r.value.structs[0].fields[0].name);
    ASSERT_STREQ("y", r.value.structs[0].fields[1].name);
}

UTEST(parse, multiple_structs) {
    char buf[8192];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "float x, y;"
        "int health;"
        "double speed;"
    "} enemy;"
    "typedef struct {"
        "char id;"
        "int score, level;"
        "float damage;"
    "} player;";
    parse_result r = parse_header(&a, header);
    ASSERT_FALSE(r.err);
    ASSERT_EQ((size_t)2, r.value.struct_count);

    // enemy: float x, float y, int health, double speed
    ASSERT_STREQ("enemy", r.value.structs[0].name);
    ASSERT_EQ((size_t)4, r.value.structs[0].fields_count);
    ASSERT_STREQ("x", r.value.structs[0].fields[0].name);
    ASSERT_EQ(ast_float, r.value.structs[0].fields[0].type);
    ASSERT_STREQ("y", r.value.structs[0].fields[1].name);
    ASSERT_EQ(ast_float, r.value.structs[0].fields[1].type);
    ASSERT_STREQ("health", r.value.structs[0].fields[2].name);
    ASSERT_EQ(ast_int, r.value.structs[0].fields[2].type);
    ASSERT_STREQ("speed", r.value.structs[0].fields[3].name);
    ASSERT_EQ(ast_double, r.value.structs[0].fields[3].type);

    // player: char id, int score, int level, float damage
    ASSERT_STREQ("player", r.value.structs[1].name);
    ASSERT_EQ((size_t)4, r.value.structs[1].fields_count);
    ASSERT_STREQ("id", r.value.structs[1].fields[0].name);
    ASSERT_EQ(ast_char, r.value.structs[1].fields[0].type);
    ASSERT_STREQ("score", r.value.structs[1].fields[1].name);
    ASSERT_EQ(ast_int, r.value.structs[1].fields[1].type);
    ASSERT_STREQ("level", r.value.structs[1].fields[2].name);
    ASSERT_EQ(ast_int, r.value.structs[1].fields[2].type);
    ASSERT_STREQ("damage", r.value.structs[1].fields[3].name);
    ASSERT_EQ(ast_float, r.value.structs[1].fields[3].type);
}

UTEST(parse, too_many_structs) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    // build a header with 65 structs
    char header[65 * 40];
    int offset = 0;
    for (int i = 0; i < 65; i++) {
        offset += sprintf(&header[offset], "typedef struct {int x;} s%d;", i);
    }
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("more than 64 structs, too many structs", r.err);
}

UTEST(parse, missing_semicolon_after_name) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "int x;"
    "} enemy";
    parse_result r = parse_header(&a, header);
    ASSERT_FALSE(r.err);
    ASSERT_STREQ("enemy", r.value.structs[0].name);
}

UTEST(parse, empty_struct) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
    "} empty;";
    parse_result r = parse_header(&a, header);
    ASSERT_FALSE(r.err);
    ASSERT_STREQ("empty", r.value.structs[0].name);
    ASSERT_EQ((size_t)0, r.value.structs[0].fields_count);
}

UTEST(parse, missing_struct_name) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {\n"
        "int x;\n"
    "} ;";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("struct missing name at line 3", r.err);
}

/*
UTEST(foo, bar) {
    char buf[4096];
    arena b;
    create_arena(&b, buf, sizeof(buf));
    char* old_header =
    "typedef struct {"
        "float x, y;"
    "} enemy;";
    // allocate 3 enemies

    char* new_header =
    "typedef struct {"
        "float x, y;"
        "int health;"
    "} enemy;";

     diff_structs(old_header, new_header);
    // migration m = generate_migration(diff);
    // migrate(m, b);
    // assert previous enemies still exist with "old values"

  ASSERT_TRUE(1);
}
*/