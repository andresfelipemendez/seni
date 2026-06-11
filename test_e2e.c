// platform impl first: on windows it pulls in windows.h, which must come
// before utest.h so utest uses windows.h's LARGE_INTEGER
#include "platform.h"
#if defined(_WIN32)
#include "platform_windows.c"
#else
#include "platform_linux.c"
#endif
#include "utest.h"
#include "seni.h"
#include "arena.h"
#include "arena.c"
#include "seni.c"

UTEST_MAIN()

static char* read_file(arena* a, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fixture not found: %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = allocate(a, (size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// writes code to build/<name>.c, compiles to build/<name>.<dll|so>, loads it.
// on gcc failure prints the generated code + gcc stderr and returns NULL.
static platform_lib compile_and_load(const char* code, const char* name) {
    char src_path[256], lib_path[256], err_path[256];
    platform_make_dir("build");
    snprintf(src_path, sizeof(src_path), "build/%s.c", name);
    snprintf(lib_path, sizeof(lib_path), "build/%s.%s", name, platform_lib_extension());
    snprintf(err_path, sizeof(err_path), "build/%s.err", name);
    FILE* f = fopen(src_path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", src_path); return NULL; }
    fputs(code, f);
    fclose(f);
    if (platform_compile_shared(src_path, lib_path, err_path) != 0) {
        fprintf(stderr, "gcc failed for %s\ngenerated code:\n%s\n", src_path, code);
        FILE* e = fopen(err_path, "rb");
        if (e) {
            char line[512];
            while (fgets(line, sizeof(line), e)) fputs(line, stderr);
            fclose(e);
        }
        return NULL;
    }
    return platform_load_lib(lib_path);
}

typedef void (*migrate_fn)(void* old_p, void* new_p, size_t count);

// pipeline: read both fixtures, diff, generate, compile, load, return migrate_<struct_name>
static migrate_fn build_migration(arena* a, const char* old_path, const char* new_path,
                                  const char* test_name, const char* struct_name, platform_lib* out_mod) {
    char* old_header = read_file(a, old_path);
    char* new_header = read_file(a, new_path);
    if (!old_header || !new_header) return NULL;
    diff_result d = diff_structs(a, old_header, new_header);
    if (d.err) { fprintf(stderr, "diff error: %s\n", d.err); return NULL; }
    generate_result g = generate_migration(a, d.value);
    if (g.err) { fprintf(stderr, "generate error: %s\n", g.err); return NULL; }
    platform_lib m = compile_and_load(g.code, test_name);
    if (!m) return NULL;
    *out_mod = m;
    char sym[128];
    snprintf(sym, sizeof(sym), "migrate_%s", struct_name);
    migrate_fn fn = (migrate_fn)platform_get_symbol(m, sym);
    if (!fn) fprintf(stderr, "symbol not found: %s\n", sym);
    return fn;
}

UTEST(e2e, add_field) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    platform_lib mod = NULL;
    migrate_fn migrate = build_migration(&a, "fixtures/enemy_v1.h", "fixtures/enemy_v2.h",
                                         "add_field", "enemy", &mod);
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float x, y; } enemy_v1;
    typedef struct { float x, y; int health; } enemy_v2;

    enemy_v1 old_block[3] = { {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f} };
    enemy_v2 new_block[3];
    memset(new_block, 0xCD, sizeof(new_block));  // poison: catches missed zero-init

    migrate(old_block, new_block, 3);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(old_block[i].x, new_block[i].x);
        ASSERT_EQ(old_block[i].y, new_block[i].y);
        ASSERT_EQ(0, new_block[i].health);
    }
    platform_unload_lib(mod);
}

UTEST(e2e, remove_field) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    platform_lib mod = NULL;
    migrate_fn migrate = build_migration(&a, "fixtures/enemy_v2.h", "fixtures/enemy_v1.h",
                                         "remove_field", "enemy", &mod);
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float x, y; int health; } enemy_v2;
    typedef struct { float x, y; } enemy_v1;

    enemy_v2 old_block[2] = { {1.0f, 2.0f, 99}, {3.0f, 4.0f, 50} };
    enemy_v1 new_block[2];
    memset(new_block, 0xCD, sizeof(new_block));

    migrate(old_block, new_block, 2);

    for (int i = 0; i < 2; i++) {
        ASSERT_EQ(old_block[i].x, new_block[i].x);
        ASSERT_EQ(old_block[i].y, new_block[i].y);
    }
    platform_unload_lib(mod);
}

UTEST(e2e, reorder_fields) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    platform_lib mod = NULL;
    migrate_fn migrate = build_migration(&a, "fixtures/reorder_v1.h", "fixtures/reorder_v2.h",
                                         "reorder_fields", "enemy", &mod);
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float x; int health; double speed; } enemy_v1;
    typedef struct { double speed; float x; int health; } enemy_v2;

    enemy_v1 old_block[2] = { {1.5f, 7, 2.25}, {3.5f, 9, 4.75} };
    enemy_v2 new_block[2];
    memset(new_block, 0xCD, sizeof(new_block));

    migrate(old_block, new_block, 2);

    for (int i = 0; i < 2; i++) {
        ASSERT_EQ(old_block[i].x, new_block[i].x);
        ASSERT_EQ(old_block[i].health, new_block[i].health);
        ASSERT_EQ(old_block[i].speed, new_block[i].speed);
    }
    platform_unload_lib(mod);
}

UTEST(e2e, multiple_structs) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    char* old_header = read_file(&a, "fixtures/world_v1.h");
    char* new_header = read_file(&a, "fixtures/world_v2.h");
    ASSERT_TRUE(old_header != NULL);
    ASSERT_TRUE(new_header != NULL);
    diff_result d = diff_structs(&a, old_header, new_header);
    ASSERT_FALSE(d.err);
    generate_result g = generate_migration(&a, d.value);
    ASSERT_FALSE(g.err);
    platform_lib mod = compile_and_load(g.code, "multiple_structs");
    ASSERT_TRUE(mod != NULL);

    migrate_fn migrate_enemy_fn = (migrate_fn)platform_get_symbol(mod, "migrate_enemy");
    migrate_fn migrate_player_fn = (migrate_fn)platform_get_symbol(mod, "migrate_player");
    ASSERT_TRUE(migrate_enemy_fn != NULL);
    ASSERT_TRUE(migrate_player_fn != NULL);

    typedef struct { float x, y; } enemy_v1;
    typedef struct { float x, y; int health; } enemy_v2;
    typedef struct { int score; } player_v1;
    typedef struct { int score, level; } player_v2;

    enemy_v1 old_enemies[2] = { {1.0f, 2.0f}, {3.0f, 4.0f} };
    enemy_v2 new_enemies[2];
    memset(new_enemies, 0xCD, sizeof(new_enemies));
    migrate_enemy_fn(old_enemies, new_enemies, 2);
    for (int i = 0; i < 2; i++) {
        ASSERT_EQ(old_enemies[i].x, new_enemies[i].x);
        ASSERT_EQ(old_enemies[i].y, new_enemies[i].y);
        ASSERT_EQ(0, new_enemies[i].health);
    }

    player_v1 old_players[1] = { {1234} };
    player_v2 new_players[1];
    memset(new_players, 0xCD, sizeof(new_players));
    migrate_player_fn(old_players, new_players, 1);
    ASSERT_EQ(1234, new_players[0].score);
    ASSERT_EQ(0, new_players[0].level);

    platform_unload_lib(mod);
}

// hot-reload scenario: the old header is not on disk anymore (the save
// overwrote it) — it lives embedded inside the currently-loaded game dll.
// build a fake game dll that embeds enemy_v1.h via seni_embed.h, pull the
// layout back out through the seni_layout symbol, diff against the new
// header file, migrate.
UTEST(e2e, layout_embedded_in_dll) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));

    const char* game_src =
        "#include \"../seni_embed.h\"\n"  // generated source lives in build/
        "SENI_EMBED_LAYOUT(\"fixtures/enemy_v1.h\");\n";
    platform_lib game = compile_and_load(game_src, "game_v1");
    ASSERT_TRUE(game != NULL);

    const char** layout_p = (const char**)platform_get_symbol(game, "seni_layout");
    ASSERT_TRUE(layout_p != NULL);
    const char* old_header = *layout_p;

    // embedded bytes must be identical to the file gcc compiled against
    char* file_header = read_file(&a, "fixtures/enemy_v1.h");
    ASSERT_TRUE(file_header != NULL);
    ASSERT_STREQ(file_header, old_header);

    char* new_header = read_file(&a, "fixtures/enemy_v2.h");
    ASSERT_TRUE(new_header != NULL);
    diff_result d = diff_structs(&a, (char*)old_header, new_header);
    ASSERT_FALSE(d.err);
    generate_result g = generate_migration(&a, d.value);
    ASSERT_FALSE(g.err);
    platform_lib mod = compile_and_load(g.code, "layout_embedded_migration");
    ASSERT_TRUE(mod != NULL);
    migrate_fn migrate = (migrate_fn)platform_get_symbol(mod, "migrate_enemy");
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float x, y; } enemy_v1;
    typedef struct { float x, y; int health; } enemy_v2;

    enemy_v1 old_block[3] = { {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f} };
    enemy_v2 new_block[3];
    memset(new_block, 0xCD, sizeof(new_block));

    migrate(old_block, new_block, 3);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(old_block[i].x, new_block[i].x);
        ASSERT_EQ(old_block[i].y, new_block[i].y);
        ASSERT_EQ(0, new_block[i].health);
    }
    platform_unload_lib(mod);
    platform_unload_lib(game);
}

UTEST(e2e, array_resize) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    platform_lib mod = NULL;
    migrate_fn migrate = build_migration(&a, "fixtures/thing_v1.h", "fixtures/thing_v2.h",
                                         "array_resize", "thing", &mod);
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float pos[4]; int id; } thing_v1;
    typedef struct { float pos[2]; int id; float vel[3]; } thing_v2;

    thing_v1 old_block[2] = {
        { {1.0f, 2.0f, 3.0f, 4.0f}, 10 },
        { {5.0f, 6.0f, 7.0f, 8.0f}, 20 },
    };
    thing_v2 new_block[2];
    memset(new_block, 0xCD, sizeof(new_block));

    migrate(old_block, new_block, 2);

    for (int i = 0; i < 2; i++) {
        ASSERT_EQ(old_block[i].pos[0], new_block[i].pos[0]);
        ASSERT_EQ(old_block[i].pos[1], new_block[i].pos[1]);
        ASSERT_EQ(old_block[i].id, new_block[i].id);
        for (int j = 0; j < 3; j++) {
            ASSERT_EQ(0.0f, new_block[i].vel[j]);
        }
    }
    platform_unload_lib(mod);
}

static int write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); return 1; }
    fputs(content, f);
    fclose(f);
    return 0;
}

typedef void (*game_fn)(void* state, size_t count);

/* full engine lifecycle: host owns the state memory, game code lives in a
   dll that gets rebuilt and hot-reloaded after the struct layout changes.

   1. working header build/game_current.h starts as game_v1.h
   2. game dll v1 (embeds layout, has game_init/game_update) loads, ticks twice
   3. the "save": game_current.h is OVERWRITTEN with the v2 layout --
      from here the old layout exists only inside the loaded v1 dll
   4. game dll v2 builds from the new header, with update logic that uses
      the new fields (health, trail[2])
   5. reload: old layout read from v1 dll's seni_layout symbol, new layout
      from v2 dll's, diff -> generate -> compile migration dll -> migrate
      host state into a new block, unload v1
   6. v2's game_update ticks the MIGRATED memory; correct values coming out
      of v2's view of the block is the layout proof */
UTEST(e2e, full_hot_reload) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));

    /* host's view of the two layouts */
    typedef struct { float x, y; float speed; } host_enemy_v1;
    typedef struct { float x, y; float speed; int health; float trail[2]; } host_enemy_v2;

    static const char* game_v1_src =
        "#include <stddef.h>\n"
        "#include \"game_current.h\"\n"
        "#include \"../seni_embed.h\"\n"
        "SENI_EMBED_LAYOUT(\"build/game_current.h\");\n"
        "#if defined(_WIN32)\n"
        "#define GEXPORT __declspec(dllexport)\n"
        "#else\n"
        "#define GEXPORT\n"
        "#endif\n"
        "GEXPORT void game_init(void* state, size_t count) {\n"
        "    enemy* e = (enemy*)state;\n"
        "    size_t i;\n"
        "    for (i = 0; i < count; i++) {\n"
        "        e[i].x = (float)(i * 10);\n"
        "        e[i].y = 0.0f;\n"
        "        e[i].speed = (float)(1 + i);\n"
        "    }\n"
        "}\n"
        "GEXPORT void game_update(void* state, size_t count) {\n"
        "    enemy* e = (enemy*)state;\n"
        "    size_t i;\n"
        "    for (i = 0; i < count; i++) { e[i].x += e[i].speed; e[i].y += 1.0f; }\n"
        "}\n";

    static const char* game_v2_src =
        "#include <stddef.h>\n"
        "#include \"game_current.h\"\n"
        "#include \"../seni_embed.h\"\n"
        "SENI_EMBED_LAYOUT(\"build/game_current.h\");\n"
        "#if defined(_WIN32)\n"
        "#define GEXPORT __declspec(dllexport)\n"
        "#else\n"
        "#define GEXPORT\n"
        "#endif\n"
        "GEXPORT void game_update(void* state, size_t count) {\n"
        "    enemy* e = (enemy*)state;\n"
        "    size_t i;\n"
        "    for (i = 0; i < count; i++) {\n"
        "        e[i].x += e[i].speed;\n"
        "        e[i].y += 1.0f;\n"
        "        e[i].health += 1;\n"
        "        e[i].trail[0] = e[i].x;\n"
        "        e[i].trail[1] = e[i].y;\n"
        "    }\n"
        "}\n";

    /* 1. working header starts at v1 */
    char* v1_header = read_file(&a, "fixtures/game_v1.h");
    ASSERT_TRUE(v1_header != NULL);
    platform_make_dir("build");
    ASSERT_EQ(0, write_file("build/game_current.h", v1_header));

    /* 2. build + load game v1, init, tick twice */
    platform_lib game_v1 = compile_and_load(game_v1_src, "hotgame_v1");
    ASSERT_TRUE(game_v1 != NULL);
    game_fn init_v1 = (game_fn)platform_get_symbol(game_v1, "game_init");
    game_fn update_v1 = (game_fn)platform_get_symbol(game_v1, "game_update");
    ASSERT_TRUE(init_v1 != NULL);
    ASSERT_TRUE(update_v1 != NULL);

    host_enemy_v1 state_v1[3];
    init_v1(state_v1, 3);
    update_v1(state_v1, 3);
    update_v1(state_v1, 3);
    /* after 2 ticks: x = 10*i + 2*(1+i), y = 2 */
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ((float)(10 * i + 2 * (1 + i)), state_v1[i].x);
        ASSERT_EQ(2.0f, state_v1[i].y);
    }

    /* 3. the save: overwrite the working header with the v2 layout.
       old layout is now ONLY inside the loaded v1 dll. */
    char* v2_header = read_file(&a, "fixtures/game_v2.h");
    ASSERT_TRUE(v2_header != NULL);
    ASSERT_EQ(0, write_file("build/game_current.h", v2_header));

    /* 4. rebuild: game dll v2 embeds the new layout */
    platform_lib game_v2 = compile_and_load(game_v2_src, "hotgame_v2");
    ASSERT_TRUE(game_v2 != NULL);
    game_fn update_v2 = (game_fn)platform_get_symbol(game_v2, "game_update");
    ASSERT_TRUE(update_v2 != NULL);

    /* 5. reload: diff the layouts the two dlls were actually built with */
    const char** old_layout_p = (const char**)platform_get_symbol(game_v1, "seni_layout");
    const char** new_layout_p = (const char**)platform_get_symbol(game_v2, "seni_layout");
    ASSERT_TRUE(old_layout_p != NULL);
    ASSERT_TRUE(new_layout_p != NULL);

    diff_result d = diff_structs(&a, (char*)*old_layout_p, (char*)*new_layout_p);
    ASSERT_FALSE(d.err);
    generate_result g = generate_migration(&a, d.value);
    ASSERT_FALSE(g.err);
    platform_lib migration = compile_and_load(g.code, "hotgame_migration");
    ASSERT_TRUE(migration != NULL);
    migrate_fn migrate = (migrate_fn)platform_get_symbol(migration, "migrate_enemy");
    ASSERT_TRUE(migrate != NULL);

    host_enemy_v2 state_v2[3];
    memset(state_v2, 0xCD, sizeof(state_v2));
    migrate(state_v1, state_v2, 3);
    platform_unload_lib(game_v1); /* old game code gone, state survived */

    /* migrated: x/y/speed carried over, health zeroed, trail zeroed */
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ((float)(10 * i + 2 * (1 + i)), state_v2[i].x);
        ASSERT_EQ(2.0f, state_v2[i].y);
        ASSERT_EQ((float)(1 + i), state_v2[i].speed);
        ASSERT_EQ(0, state_v2[i].health);
        ASSERT_EQ(0.0f, state_v2[i].trail[0]);
        ASSERT_EQ(0.0f, state_v2[i].trail[1]);
    }

    /* 6. v2 game code ticks the migrated memory */
    update_v2(state_v2, 3);
    for (int i = 0; i < 3; i++) {
        float expect_x = (float)(10 * i + 3 * (1 + i));
        ASSERT_EQ(expect_x, state_v2[i].x);
        ASSERT_EQ(3.0f, state_v2[i].y);
        ASSERT_EQ(1, state_v2[i].health);
        ASSERT_EQ(expect_x, state_v2[i].trail[0]);
        ASSERT_EQ(3.0f, state_v2[i].trail[1]);
    }

    platform_unload_lib(migration);
    platform_unload_lib(game_v2);
}

UTEST(e2e, identical) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    platform_lib mod = NULL;
    migrate_fn migrate = build_migration(&a, "fixtures/enemy_v1.h", "fixtures/enemy_v1.h",
                                         "identical", "enemy", &mod);
    ASSERT_TRUE(migrate != NULL);

    typedef struct { float x, y; } enemy_v1;

    enemy_v1 old_block[3] = { {1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f} };
    enemy_v1 new_block[3];
    memset(new_block, 0xCD, sizeof(new_block));

    migrate(old_block, new_block, 3);

    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(old_block[i].x, new_block[i].x);
        ASSERT_EQ(old_block[i].y, new_block[i].y);
    }
    platform_unload_lib(mod);
}
