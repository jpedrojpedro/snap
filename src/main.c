#include <stdio.h>
#include <string.h>
#include "lua_dw.h"

int main(int argc, char *argv[]) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_dw(L);
    lua_setglobal(L, "dw");

    if (argc > 1) {
        if (luaL_dofile(L, argv[1]) != LUA_OK) {
            fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
            lua_close(L);
            return 1;
        }
    } else {
        /* Simple REPL */
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

    lua_close(L);
    return 0;
}
