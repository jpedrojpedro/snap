#include <stdio.h>
#include "lua_dw.h"

static const char *bootstrap = "require('main')";

int main(int argc, char *argv[]) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    luaopen_dw(L);
    lua_setglobal(L, "snap");

    /* Pass argv to Lua as global 'arg' table */
    lua_newtable(L);
    for (int i = 1; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
    }
    lua_pushinteger(L, argc - 1);
    lua_setfield(L, -2, "n");
    lua_setglobal(L, "arg");

    /* Add src/lua_scripts to package.path */
    luaL_dostring(L, "package.path = 'src/lua_scripts/?.lua;' .. package.path");

    if (luaL_dostring(L, bootstrap) != LUA_OK) {
        fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}
