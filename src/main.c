#include <stdio.h>
#include "lua_dw.h"

static const char *embedded_main =
"-- Expose shortcuts globally\n"
"read = snap.read\n"
"meta = snap.meta\n"
"query = snap.query\n"
"profile = snap.profile\n"
"export = snap.export\n"
"help = snap.help\n"
"\n"
"local sql_keywords = {\n"
"    SELECT=true, INSERT=true, UPDATE=true, DELETE=true, CREATE=true,\n"
"    DROP=true, ALTER=true, WITH=true, COPY=true, SUMMARIZE=true,\n"
"    DESCRIBE=true, SHOW=true, EXPLAIN=true, PRAGMA=true, ATTACH=true,\n"
"}\n"
"\n"
"local function is_sql(line)\n"
"    local first = line:match('^%s*(%w+)')\n"
"    return first and sql_keywords[first:upper()] or false\n"
"end\n"
"\n"
"local function is_data_file(path)\n"
"    return path:match('%.[Cc][Ss][Vv]$') or path:match('%.[Pp][Aa][Rr][Qq][Uu][Ee][Tt]$')\n"
"        or path:match('%.[Jj][Ss][Oo][Nn]$') or path:match('%.db$') or path:match('%.sqlite$')\n"
"end\n"
"\n"
"local function dirname(path)\n"
"    return path:match('(.+)/[^/]+$') or '.'\n"
"end\n"
"\n"
"local function folder_name(path)\n"
"    local dir = dirname(path)\n"
"    local name = dir:match('([^/]+)$') or dir\n"
"    return name:gsub('-', '_')\n"
"end\n"
"\n"
"local function load_files(files)\n"
"    if #files == 1 then\n"
"        read(files[1])\n"
"    else\n"
"        local dir = dirname(files[1])\n"
"        local name = folder_name(files[1])\n"
"        local ext = files[1]:match('(%.[^.]+)$') or '.csv'\n"
"        local reader = 'read_csv_auto'\n"
"        if ext:lower() == '.parquet' then reader = 'read_parquet'\n"
"        elseif ext:lower() == '.json' then reader = 'read_json_auto' end\n"
"        local sql = string.format(\n"
"            'CREATE OR REPLACE VIEW \"%s\" AS SELECT * EXCLUDE(filename), replace(filename, \\'%s/\\', \\'\\') AS source FROM %s(\\'%s/*%s\\', filename=true)',\n"
"            name, dir, reader, dir, ext\n"
"        )\n"
"        query(sql)\n"
"        print(string.format(\"  -> View '%s' created from %d files\", name, #files))\n"
"    end\n"
"end\n"
"\n"
"local function repl()\n"
"    local buffer = ''\n"
"    io.write('snap> ')\n"
"    for line in io.lines() do\n"
"        if buffer ~= '' then\n"
"            buffer = buffer .. '\\n' .. line\n"
"            if line:match(';%s*$') then\n"
"                local ok, err = pcall(query, buffer)\n"
"                if not ok then io.stderr:write(err .. '\\n') end\n"
"                buffer = ''\n"
"            else\n"
"                io.write('  ... ')\n"
"                goto continue\n"
"            end\n"
"        elseif is_sql(line) then\n"
"            if line:match(';%s*$') then\n"
"                local ok, err = pcall(query, line)\n"
"                if not ok then io.stderr:write(err .. '\\n') end\n"
"            else\n"
"                buffer = line\n"
"                io.write('  ... ')\n"
"                goto continue\n"
"            end\n"
"        else\n"
"            local fn, err = load(line)\n"
"            if fn then\n"
"                local ok, rerr = pcall(fn)\n"
"                if not ok then io.stderr:write(rerr .. '\\n') end\n"
"            else\n"
"                io.stderr:write(err .. '\\n')\n"
"            end\n"
"        end\n"
"        io.write('snap> ')\n"
"        ::continue::\n"
"    end\n"
"end\n"
"\n"
"local files = {}\n"
"for i = 1, arg.n do\n"
"    files[#files + 1] = arg[i]\n"
"end\n"
"\n"
"if #files == 0 then\n"
"    repl()\n"
"elseif is_data_file(files[1]) then\n"
"    load_files(files)\n"
"    repl()\n"
"else\n"
"    dofile(files[1])\n"
"end\n"
;

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

    if (luaL_dostring(L, embedded_main) != LUA_OK) {
        fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}
