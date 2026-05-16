Here is a comprehensive blueprint designed to be handed directly to an agentic coding assistant like Claude Code. It outlines the project architecture, directory structure, data flow, and exact step-by-step implementation phases required to build your zero-dependency local analytics monolith.

---

# Architecture Specification: Portable Data Analytics Monolith (`dw`)

This specification details how to build a single-binary, zero-dependency command-line utility and Lua runtime embedding **Lua (or LuaJIT)**, **DuckDB**, and **SQLite3**.

```
                       +---------------------------------------+
                       |          Portable CLI / Script        |
                       +---------------------------------------+
                                           |
                                           v
                       +---------------------------------------+
                       |       Lua 5.4 / LuaJIT Runtime        |
                       +---------------------------------------+
                                           |
                    +----------------------+----------------------+
                    |                                             |
                    v                                             v
        +-----------------------+                     +-----------------------+
        |  DuckDB (C Engine)    |                     |  SQLite3 (C Engine)   |
        +-----------------------+                     +-----------------------+
        |  - Columnar OLAP      |                     |  - Row-oriented OLTP  |
        |  - Parquet/CSV Ingest |                     |  - Metadata & Auditing|
        +-----------------------+                     +-----------------------+

```

## 1. Project Directory Structure

Instruct Claude Code to set up the repository workspace with this layout:

```text
.
├── Makefile                     # Builds the unified binary monolith
├── src/
│   ├── main.c                   # C Entrypoint: initializes VMs and registers drivers
│   ├── lua_dw.c                 # C bindings mapping the core 'dw' API to Lua
│   ├── lua_dw.h                 # Headers for 'dw' core module
│   └── lua_scripts/
│       ├── init.lua             # Internal bootstrap layer script (embedded in binary)
│       └── formatters.lua       # Tabular/Markdown/JSON output rendering utilities
├── vendor/                      # Unzipped source dependencies for absolute portability
│   ├── lua-5.4.x/               # Raw Lua source code
│   ├── duckdb/                  # duckdb.h and libduckdb dynamic/static library
│   └── sqlite/                  # sqlite3.h and sqlite3.c amalgamation
└── tests/
    └── test_pipeline.lua        # Integration verification script

```

---

## 2. Component Integration Strategy

### Execution Flow

1. **Host Bootstrapping:** The compiled C program compiles and initializes a standard Lua State variable (`lua_State *L`).
2. **Static Linking:** SQLite (`sqlite3.c`) is compiled directly into the binary compilation unit. DuckDB is integrated via its official C API (`libduckdb`).
3. **Module Injection:** The C host registers a global Lua module named `dw`. It maps the C language execution of DuckDB and SQLite calls straight into Lua functions using standard stack manipulation (`lua_pushcfunction`).

---

## 3. Step-by-Step Implementation Blueprint

Provide the following sequence directly to Claude Code:

### Phase 1: Dependency Vendoring & Compilation Scaffold

* **Step 1.1:** Fetch the SQLite Amalgamation source code (`sqlite3.c` and `sqlite3.h`).
* **Step 1.2:** Fetch the DuckDB C-API headers and precompiled static library appropriate for the target compilation architecture.
* **Step 1.3:** Download the target Lua source release (e.g., Lua 5.4.x) into the `vendor/` directory.
* **Step 1.4:** Generate a `Makefile` that compiles the Lua source, bundles the SQLite unit, references the DuckDB library, and links everything statically into a single compilation target (`dw`).

### Phase 2: Design the C Core Engine (`src/main.c`)

* **Step 2.1:** Write the foundational `main` entry point. It must spin up a standard `luaL_newstate()` and open standard core libraries (`luaL_openlibs`).
* **Step 2.2:** Build an internal runner block: if an argument is passed (e.g., `./dw pipeline.lua`), read and execute that file via `luaL_dofile`. If no arguments are passed, launch a basic Read-Eval-Print Loop (REPL).
* **Step 2.3:** Write the native binding layer (`src/lua_dw.c`). This binds your underlying database handles safely to the Lua lifecycle. Ensure an implicit in-memory or file-backed SQLite descriptor and DuckDB descriptor are initialized instantly when the runtime starts up.

### Phase 3: Build the Lua-Facing Native API Layer

Implement the following bindings mapping Lua calls directly down to the database engine execution targets:

#### `dw.read(path_str)`

* **Internal Logic:**
1. Inspect the incoming string path via standard filesystem heuristics.
2. If the path points to a file ending in `.db` or `.sqlite`, issue an internal DuckDB command: `ATTACH 'path' AS sqlite_db (TYPE SQLITE);`. Also bind a native raw connection to your SQLite context.
3. If the path is a folder or a discrete data file (`.parquet`, `.csv`, `.json`), deduce the file's basename (e.g., `yellow_tripdata`). Automatically issue an implicit view construction statement over DuckDB: `CREATE OR REPLACE VIEW <basename> AS SELECT * FROM read_auto('<path>');`.



#### `dw.meta()`

* **Internal Logic:**
1. Direct a query to DuckDB's catalog interface: `SELECT table_name, column_name, data_type FROM information_schema.columns WHERE table_schema = 'main';`.
2. Collect the output structures, iterate over them, and pipe the structured results back to standard output in an organized, readable list layout.



#### `dw.query(sql_str, format_str)`

* **Internal Logic:**
1. Pass the raw `sql_str` statement to `duckdb_query`.
2. Collect the column count, row schemas, and columnar output blocks.
3. Look at `format_str` (`"table"`, `"markdown"`, `"json"`). Route the result matrix into `src/lua_scripts/formatters.lua` to calculate string widths or format valid JSON, then dump the payload to `stdout`.
4. **Auditing Layer Bonus:** Use your SQLite connection handle to execute an background logging update: `INSERT INTO query_history (query, executed_at) VALUES (?, CURRENT_TIMESTAMP);`.



#### `dw.profile(target_name)`

* **Internal Logic:**
1. Formulate an execution wrapper string: `SUMMARIZE <target_name>;`.
2. Run this string through the internal `dw.query` infrastructure to render a quick, detailed statistical profile of the target table or view layout.



#### `dw.export(target_path, sql_or_table)`

* **Internal Logic:**
1. Inspect if `sql_or_table` is a simple string identifier (a view/table) or contains space tokens (raw SQL).
2. Formulate an optimization copy statement: `COPY (<sql_or_table>) TO '<target_path>' (FORMAT 'PARQUET');` (infer format type directly from the file extension string).



---

## 4. Verification Test Harness

Instruct Claude Code to generate this automated validation script inside `tests/test_pipeline.lua` to ensure correctness across the entire stack:

```lua
-- test_pipeline.lua
print("--- Starting Monolith Verification Script ---")

-- 1. Create dummy mock data via SQL query
print("[Test 1] Initializing mock data file...")
dw.query([[
    COPY (
        SELECT 1 as id, 'Alice' as name, 'Data Engineer' as role, 10500.00 as salary
        UNION ALL
        SELECT 2 as id, 'Bob' as name, 'Architect' as role, 12000.00 as salary
        UNION ALL
        SELECT 3 as id, 'Charlie' as name, 'Analyst' as role, 8500.00 as salary
    ) TO 'tests/mock_staff.parquet' (FORMAT 'PARQUET');
]])

-- 2. Validate auto-inference load capability
print("[Test 2] Testing auto-inference loader...")
dw.read("tests/mock_staff.parquet")

-- 3. Assert schema evaluation capabilities
print("[Test 3] Printing database schema metadata:")
dw.meta()

-- 4. Assert Analytical Data Transformation & Markdown output formatting
print("[Test 4] Executing analytical aggregation query:")
local query_str = [[
    SELECT 
        role, 
        COUNT(*) as total_employees, 
        ROUND(AVG(salary), 2) as average_salary 
    FROM mock_staff 
    GROUP BY role 
    ORDER BY average_salary DESC;
]]
dw.query(query_str, "markdown")

-- 5. Test statistical profile command
print("[Test 5] Running automated target profiling:")
dw.profile("mock_staff")

-- 6. Verify export facilities
print("[Test 6] Testing pipeline data exporting capabilities...")
dw.export("tests/high_earners.csv", "SELECT * FROM mock_staff WHERE salary > 9000")

print("--- Integration Tests Successfully Completed ---")

```
