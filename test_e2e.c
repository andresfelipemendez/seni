#include <windows.h>  // must come before utest.h so utest uses windows.h's LARGE_INTEGER
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

// writes code to build/<name>.c, compiles to build/<name>.dll, loads it.
// on gcc failure prints the generated code + gcc stderr and returns NULL.
static HMODULE compile_and_load(const char* code, const char* name) {
    char src_path[256], dll_path[256], err_path[256], cmd[1024];
    CreateDirectoryA("build", NULL);
    snprintf(src_path, sizeof(src_path), "build/%s.c", name);
    snprintf(dll_path, sizeof(dll_path), "build/%s.dll", name);
    snprintf(err_path, sizeof(err_path), "build/%s.err", name);
    FILE* f = fopen(src_path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", src_path); return NULL; }
    fputs(code, f);
    fclose(f);
    snprintf(cmd, sizeof(cmd), "gcc -shared -o %s %s 2> %s", dll_path, src_path, err_path);
    if (system(cmd) != 0) {
        fprintf(stderr, "gcc failed for %s\ngenerated code:\n%s\n", src_path, code);
        FILE* e = fopen(err_path, "rb");
        if (e) {
            char line[512];
            while (fgets(line, sizeof(line), e)) fputs(line, stderr);
            fclose(e);
        }
        return NULL;
    }
    HMODULE m = LoadLibraryA(dll_path);
    if (!m) fprintf(stderr, "LoadLibrary failed for %s (error %lu)\n", dll_path, GetLastError());
    return m;
}

typedef void (*migrate_fn)(void* old_p, void* new_p, size_t count);

// pipeline: read both fixtures, diff, generate, compile, load, return migrate_<struct_name>
static migrate_fn build_migration(arena* a, const char* old_path, const char* new_path,
                                  const char* test_name, const char* struct_name, HMODULE* out_mod) {
    char* old_header = read_file(a, old_path);
    char* new_header = read_file(a, new_path);
    if (!old_header || !new_header) return NULL;
    diff_result d = diff_structs(a, old_header, new_header);
    if (d.err) { fprintf(stderr, "diff error: %s\n", d.err); return NULL; }
    generate_result g = generate_migration(a, d.value);
    if (g.err) { fprintf(stderr, "generate error: %s\n", g.err); return NULL; }
    HMODULE m = compile_and_load(g.code, test_name);
    if (!m) return NULL;
    *out_mod = m;
    char sym[128];
    snprintf(sym, sizeof(sym), "migrate_%s", struct_name);
    migrate_fn fn = (migrate_fn)(void*)GetProcAddress(m, sym);
    if (!fn) fprintf(stderr, "GetProcAddress failed for symbol %s\n", sym);
    return fn;
}

UTEST(e2e, add_field) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    HMODULE mod = NULL;
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
    FreeLibrary(mod);
}

UTEST(e2e, remove_field) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    HMODULE mod = NULL;
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
    FreeLibrary(mod);
}

UTEST(e2e, reorder_fields) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    HMODULE mod = NULL;
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
    FreeLibrary(mod);
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
    HMODULE mod = compile_and_load(g.code, "multiple_structs");
    ASSERT_TRUE(mod != NULL);

    migrate_fn migrate_enemy_fn = (migrate_fn)(void*)GetProcAddress(mod, "migrate_enemy");
    migrate_fn migrate_player_fn = (migrate_fn)(void*)GetProcAddress(mod, "migrate_player");
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

    FreeLibrary(mod);
}

UTEST(e2e, identical) {
    char buf[16384];
    arena a;
    create_arena(&a, buf, sizeof(buf));
    HMODULE mod = NULL;
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
    FreeLibrary(mod);
}
