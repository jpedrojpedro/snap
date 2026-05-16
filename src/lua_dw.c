#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "lua_dw.h"

static duckdb_database db;
static duckdb_connection conn;
static sqlite3 *sdb;

static void ensure_init(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;
    if (duckdb_open(NULL, &db) == DuckDBError) {
        fprintf(stderr, "Failed to open DuckDB\n");
        exit(1);
    }
    if (duckdb_connect(db, &conn) == DuckDBError) {
        fprintf(stderr, "Failed to connect DuckDB\n");
        exit(1);
    }
    if (sqlite3_open(":memory:", &sdb) != SQLITE_OK) {
        fprintf(stderr, "Failed to open SQLite\n");
        exit(1);
    }
    sqlite3_exec(sdb, "CREATE TABLE IF NOT EXISTS query_history(query TEXT, executed_at TEXT);", NULL, NULL, NULL);
}

/* Helper: extract basename without extension */
static void basename_no_ext(const char *path, char *out, size_t sz) {
    char *tmp = strdup(path);
    char *base = basename(tmp);
    strncpy(out, base, sz - 1);
    out[sz - 1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
    free(tmp);
}

/* Helper: get file extension */
static const char *get_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

/* Helper: print duckdb result as table/markdown */
static void print_result(duckdb_result *res, const char *fmt) {
    idx_t cols = duckdb_column_count(res);
    idx_t rows = duckdb_row_count(res);
    int json = (fmt && strcmp(fmt, "json") == 0);

    if (json) {
        printf("[\n");
        for (idx_t r = 0; r < rows; r++) {
            printf("  {");
            for (idx_t c = 0; c < cols; c++) {
                char *val = duckdb_value_varchar(res, c, r);
                printf("\"%s\": \"%s\"", duckdb_column_name(res, c), val ? val : "NULL");
                if (val) duckdb_free(val);
                if (c < cols - 1) printf(", ");
            }
            printf("}%s\n", r < rows - 1 ? "," : "");
        }
        printf("]\n");
        return;
    }

    /* Compute column widths */
    int *widths = calloc(cols, sizeof(int));
    for (idx_t c = 0; c < cols; c++) {
        int len = (int)strlen(duckdb_column_name(res, c));
        widths[c] = len;
    }
    /* Sample rows for width */
    for (idx_t r = 0; r < rows && r < 100; r++) {
        for (idx_t c = 0; c < cols; c++) {
            char *val = duckdb_value_varchar(res, c, r);
            if (val) {
                int len = (int)strlen(val);
                if (len > widths[c]) widths[c] = len;
                duckdb_free(val);
            }
        }
    }

    /* Header */
    printf("|");
    for (idx_t c = 0; c < cols; c++)
        printf(" %-*s |", widths[c], duckdb_column_name(res, c));
    printf("\n|");
    for (idx_t c = 0; c < cols; c++) {
        for (int i = 0; i < widths[c] + 2; i++) printf("-");
        printf("|");
    }
    printf("\n");

    /* Rows */
    for (idx_t r = 0; r < rows; r++) {
        printf("|");
        for (idx_t c = 0; c < cols; c++) {
            char *val = duckdb_value_varchar(res, c, r);
            printf(" %-*s |", widths[c], val ? val : "NULL");
            if (val) duckdb_free(val);
        }
        printf("\n");
    }
    free(widths);
}

/* dw.read(path) */
static int l_read(lua_State *L) {
    ensure_init();
    const char *path = luaL_checkstring(L, 1);
    const char *ext = get_ext(path);

    if (strcmp(ext, "db") == 0 || strcmp(ext, "sqlite") == 0) {
        char sql[2048];
        snprintf(sql, sizeof(sql), "ATTACH '%s' AS sqlite_db (TYPE SQLITE);", path);
        duckdb_result res;
        if (duckdb_query(conn, sql, &res) == DuckDBError) {
            lua_pushstring(L, duckdb_result_error(&res));
            duckdb_destroy_result(&res);
            return lua_error(L);
        }
        duckdb_destroy_result(&res);
    } else {
        char name[256];
        basename_no_ext(path, name, sizeof(name));
        char sql[2048];
        snprintf(sql, sizeof(sql),
            "CREATE OR REPLACE VIEW %s AS SELECT * FROM read_csv_auto('%s');", name, path);
        if (strcmp(ext, "parquet") == 0)
            snprintf(sql, sizeof(sql),
                "CREATE OR REPLACE VIEW %s AS SELECT * FROM read_parquet('%s');", name, path);
        else if (strcmp(ext, "json") == 0)
            snprintf(sql, sizeof(sql),
                "CREATE OR REPLACE VIEW %s AS SELECT * FROM read_json_auto('%s');", name, path);

        duckdb_result res;
        if (duckdb_query(conn, sql, &res) == DuckDBError) {
            lua_pushstring(L, duckdb_result_error(&res));
            duckdb_destroy_result(&res);
            return lua_error(L);
        }
        duckdb_destroy_result(&res);
        printf("  -> View '%s' created from %s\n", name, path);
    }
    return 0;
}

/* snap.meta(table_name?) */
static int l_meta(lua_State *L) {
    ensure_init();
    const char *table = luaL_optstring(L, 1, NULL);
    char sql[2048];
    if (table)
        snprintf(sql, sizeof(sql),
            "SELECT column_name, data_type FROM information_schema.columns WHERE table_schema = 'main' AND table_name = '%s';", table);
    else
        snprintf(sql, sizeof(sql),
            "SELECT table_name, column_name, data_type FROM information_schema.columns WHERE table_schema = 'main';");
    duckdb_result res;
    if (duckdb_query(conn, sql, &res) == DuckDBError) {
        lua_pushstring(L, duckdb_result_error(&res));
        duckdb_destroy_result(&res);
        return lua_error(L);
    }
    print_result(&res, "table");
    duckdb_destroy_result(&res);
    return 0;
}

/* dw.query(sql, format) */
static int l_query(lua_State *L) {
    ensure_init();
    const char *sql = luaL_checkstring(L, 1);
    const char *fmt = luaL_optstring(L, 2, "table");

    duckdb_result res;
    if (duckdb_query(conn, sql, &res) == DuckDBError) {
        lua_pushstring(L, duckdb_result_error(&res));
        duckdb_destroy_result(&res);
        return lua_error(L);
    }

    if (duckdb_row_count(&res) > 0)
        print_result(&res, fmt);

    duckdb_destroy_result(&res);

    /* Audit to SQLite */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(sdb, "INSERT INTO query_history(query, executed_at) VALUES(?, datetime('now'));", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, sql, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return 0;
}

/* dw.profile(target) */
static int l_profile(lua_State *L) {
    ensure_init();
    const char *target = luaL_checkstring(L, 1);
    char sql[1024];
    snprintf(sql, sizeof(sql), "SUMMARIZE %s;", target);

    duckdb_result res;
    if (duckdb_query(conn, sql, &res) == DuckDBError) {
        lua_pushstring(L, duckdb_result_error(&res));
        duckdb_destroy_result(&res);
        return lua_error(L);
    }
    print_result(&res, "table");
    duckdb_destroy_result(&res);
    return 0;
}

/* dw.export(path, sql_or_table) */
static int l_export(lua_State *L) {
    ensure_init();
    const char *path = luaL_checkstring(L, 1);
    const char *source = luaL_checkstring(L, 2);
    const char *ext = get_ext(path);

    /* Determine format from extension */
    const char *format = "CSV";
    if (strcmp(ext, "parquet") == 0) format = "PARQUET";
    else if (strcmp(ext, "json") == 0) format = "JSON";

    /* Determine if source is SQL (contains spaces) or table name */
    int is_sql = (strchr(source, ' ') != NULL);

    char sql[4096];
    if (is_sql)
        snprintf(sql, sizeof(sql), "COPY (%s) TO '%s' (FORMAT '%s');", source, path, format);
    else
        snprintf(sql, sizeof(sql), "COPY %s TO '%s' (FORMAT '%s');", source, path, format);

    duckdb_result res;
    if (duckdb_query(conn, sql, &res) == DuckDBError) {
        lua_pushstring(L, duckdb_result_error(&res));
        duckdb_destroy_result(&res);
        return lua_error(L);
    }
    duckdb_destroy_result(&res);
    printf("  -> Exported to %s (%s)\n", path, format);
    return 0;
}

/* snap.help() */
static int l_help(lua_State *L) {
    (void)L;
    printf("\n");
    printf("  snap.read(path)            Load a file as a queryable view (parquet/csv/json/db)\n");
    printf("  snap.meta([table])         Show schema for all tables, or a specific table\n");
    printf("  snap.query(sql [,fmt])     Execute SQL. Format: \"table\", \"markdown\", \"json\"\n");
    printf("  snap.profile(table)        Statistical profiling (min, max, avg, std, quantiles)\n");
    printf("  snap.export(path, source)  Export table or query results to file\n");
    printf("  snap.help()                Show this help message\n");
    printf("\n");
    printf("  In the REPL, all functions work without the 'snap.' prefix.\n");
    printf("  SQL statements are executed directly (no quotes needed).\n");
    printf("  Multi-line SQL is supported — just end with a semicolon.\n");
    printf("\n");
    return 0;
}

static const struct luaL_Reg dw_funcs[] = {
    {"read", l_read},
    {"meta", l_meta},
    {"query", l_query},
    {"profile", l_profile},
    {"export", l_export},
    {"help", l_help},
    {NULL, NULL}
};

int luaopen_dw(lua_State *L) {
    luaL_newlib(L, dw_funcs);
    return 1;
}
