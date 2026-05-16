#include <stdio.h>
#include <string.h>
#include "lua_dw.h"

static int is_data_file(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".csv") == 0 || strcmp(dot, ".parquet") == 0 ||
            strcmp(dot, ".json") == 0 || strcmp(dot, ".db") == 0 ||
            strcmp(dot, ".sqlite") == 0);
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
            /* Load all data file arguments, then open REPL */
            for (int i = 1; i < argc; i++) {
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), "dw.read(\"%s\")", argv[i]);
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
