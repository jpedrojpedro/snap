#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "lua_dw.h"

static int is_data_file(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".csv") == 0 || strcmp(dot, ".parquet") == 0 ||
            strcmp(dot, ".json") == 0 || strcmp(dot, ".db") == 0 ||
            strcmp(dot, ".sqlite") == 0);
}

static void get_dirname(const char *path, char *out, size_t sz) {
    char *tmp = strdup(path);
    char *dir = dirname(tmp);
    char *base = basename(dir);
    strncpy(out, base, sz - 1);
    out[sz - 1] = '\0';
    free(tmp);
}

static void repl(lua_State *L) {
    char buf[4096];
    printf("snap> ");
    while (fgets(buf, sizeof(buf), stdin)) {
        if (luaL_dostring(L, buf) != LUA_OK) {
            fprintf(stderr, "%s\n", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        printf("snap> ");
    }
}

int main(int argc, char *argv[]) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_dw(L);
    lua_setglobal(L, "dw");

    if (argc > 1) {
        if (is_data_file(argv[1])) {
            if (argc > 2) {
                /* Multiple files: combine into single view using glob */
                char dir[256];
                get_dirname(argv[1], dir, sizeof(dir));
                const char *ext = strrchr(argv[1], '.');
                char cmd[8192];
                snprintf(cmd, sizeof(cmd),
                    "dw.query(\"CREATE OR REPLACE VIEW \\\"%s\\\" AS SELECT * EXCLUDE(filename), replace(filename, '%s/', '') AS source FROM read_csv_auto('%s/*%s', filename=true)\")",
                    dir, dirname(strdup(argv[1])), dirname(strdup(argv[1])), ext ? ext : ".csv");
                if (luaL_dostring(L, cmd) != LUA_OK) {
                    fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                } else {
                    printf("  -> View '%s' created from %d files\n", dir, argc - 1);
                }
            } else {
                /* Single file */
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), "dw.read(\"%s\")", argv[1]);
                if (luaL_dostring(L, cmd) != LUA_OK) {
                    fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
            repl(L);
        } else {
            /* Treat as Lua script */
            if (luaL_dofile(L, argv[1]) != LUA_OK) {
                fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
                lua_close(L);
                return 1;
            }
        }
    } else {
        repl(L);
    }

    lua_close(L);
    return 0;
}
