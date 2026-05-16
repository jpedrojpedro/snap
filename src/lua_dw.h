#ifndef LUA_DW_H
#define LUA_DW_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <duckdb.h>
#include <sqlite3.h>

int luaopen_dw(lua_State *L);

#endif
