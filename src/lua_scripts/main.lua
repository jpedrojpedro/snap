-- main.lua: snap entrypoint

-- Expose shortcuts globally (no snap. prefix needed)
read = snap.read
meta = snap.meta
query = snap.query
profile = snap.profile
export = snap.export

-- SQL keywords that trigger auto-query sugar
local sql_keywords = {
    SELECT=true, INSERT=true, UPDATE=true, DELETE=true, CREATE=true,
    DROP=true, ALTER=true, WITH=true, COPY=true, SUMMARIZE=true,
    DESCRIBE=true, SHOW=true, EXPLAIN=true, PRAGMA=true, ATTACH=true,
}

local function is_sql(line)
    local first = line:match("^%s*(%w+)")
    return first and sql_keywords[first:upper()] or false
end

local function is_data_file(path)
    return path:match("%.[Cc][Ss][Vv]$") or path:match("%.[Pp][Aa][Rr][Qq][Uu][Ee][Tt]$")
        or path:match("%.[Jj][Ss][Oo][Nn]$") or path:match("%.db$") or path:match("%.sqlite$")
end

local function dirname(path)
    return path:match("(.+)/[^/]+$") or "."
end

local function basename_no_ext(path)
    local base = path:match("([^/]+)$") or path
    return base:match("(.+)%..+$") or base
end

local function folder_name(path)
    local dir = dirname(path)
    return dir:match("([^/]+)$") or dir
end

local function load_files(files)
    if #files == 1 then
        read(files[1])
    else
        -- Multiple files: combine into single view named after folder
        local dir = dirname(files[1])
        local name = folder_name(files[1])
        local ext = files[1]:match("(%.[^.]+)$") or ".csv"
        local sql = string.format(
            [[CREATE OR REPLACE VIEW "%s" AS SELECT * EXCLUDE(filename), replace(filename, '%s/', '') AS source FROM read_csv_auto('%s/*%s', filename=true)]],
            name, dir, dir, ext
        )
        if ext:lower() == ".parquet" then
            sql = string.format(
                [[CREATE OR REPLACE VIEW "%s" AS SELECT * EXCLUDE(filename), replace(filename, '%s/', '') AS source FROM read_parquet('%s/*%s', filename=true)]],
                name, dir, dir, ext
            )
        elseif ext:lower() == ".json" then
            sql = string.format(
                [[CREATE OR REPLACE VIEW "%s" AS SELECT * EXCLUDE(filename), replace(filename, '%s/', '') AS source FROM read_json_auto('%s/*%s', filename=true)]],
                name, dir, dir, ext
            )
        end
        query(sql)
        print(string.format("  -> View '%s' created from %d files", name, #files))
    end
end

local function repl()
    local buffer = ""
    io.write("snap> ")
    for line in io.lines() do
        -- Multi-line SQL: accumulate until semicolon
        if buffer ~= "" then
            buffer = buffer .. "\n" .. line
            if line:match(";%s*$") then
                query(buffer)
                buffer = ""
            else
                io.write("  ... ")
                goto continue
            end
        elseif is_sql(line) then
            -- SQL sugar: if no trailing semicolon, start multi-line
            if line:match(";%s*$") then
                query(line)
            else
                buffer = line
                io.write("  ... ")
                goto continue
            end
        else
            -- Regular Lua
            local fn, err = load(line)
            if fn then
                local ok, rerr = pcall(fn)
                if not ok then io.stderr:write(rerr .. "\n") end
            else
                io.stderr:write(err .. "\n")
            end
        end
        io.write("snap> ")
        ::continue::
    end
end

-- Main
local files = {}
for i = 1, arg.n do
    files[#files + 1] = arg[i]
end

if #files == 0 then
    repl()
elseif is_data_file(files[1]) then
    load_files(files)
    repl()
else
    -- Lua script
    dofile(files[1])
end
