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
    ast parsed = parse_header(&a, header);
    ASSERT_EQ((size_t)1,parsed.struct_count);
    ASSERT_EQ((size_t)2,parsed.structs[0].fields_count);
    ASSERT_STREQ("enemy",parsed.structs[0].name);
    ASSERT_STREQ("x",parsed.structs[0].fields[0].name);
    ASSERT_EQ(ast_float,parsed.structs[0].fields[0].type);
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