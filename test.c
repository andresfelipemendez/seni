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

// Bug #1: unknown type returns error
UTEST(parse, unknown_type) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "unsigned int x;"
    "} enemy;";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("unknown type", r.err);
}

// Bug #2: missing semicolon returns error instead of reading past buffer
UTEST(parse, missing_semicolon) {
    char buf[4096];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* header =
    "typedef struct {"
        "int x";
    parse_result r = parse_header(&a, header);
    ASSERT_TRUE(r.err);
    ASSERT_STREQ("unexpected end of input", r.err);
}

// Bug #5: out of memory returns error
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

// Bug #6: trailing spaces in field names
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