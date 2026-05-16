# SNAP

**Simple Native Analytics Portable** — instantly read parquet files with zero setup.

A single-binary local data analytics utility embedding Lua 5.4, DuckDB, and SQLite3. No dependencies to install, no configuration files, no Python environments. Just run queries against your data files.

```
./snap data.parquet
snap> SELECT * FROM data LIMIT 10;
```

## Features

- **Zero setup** — one binary, runs anywhere
- **Auto-inference** — point at `.parquet`, `.csv`, or `.json` files and query immediately
- **DuckDB engine** — full analytical SQL (window functions, CTEs, aggregations)
- **SQLite audit log** — every query is automatically logged with timestamps
- **Built-in profiling** — statistical summaries with a single command
- **Multiple output formats** — table, markdown, JSON
- **Export pipeline** — write query results to parquet/csv/json with format auto-detection

## Quick Start

```bash
make
```

Point at your data files — they're auto-loaded as queryable views:

```bash
./snap sales.parquet
  -> View 'sales' created from sales.parquet
snap> SELECT region, SUM(amount) FROM sales GROUP BY region;
```

Load multiple files at once:

```bash
./snap data/*.csv
  -> View 'customers' created from data/customers.csv
  -> View 'orders' created from data/orders.csv
snap> SELECT * FROM customers JOIN orders USING(id);
```

Run a Lua pipeline script:

```bash
./snap my_pipeline.lua
```

Or just open the REPL:

```bash
./snap
snap> read("sales.parquet")
snap> SELECT region, SUM(amount) FROM sales GROUP BY region;
```

> **macOS note:** If Gatekeeper blocks execution, run `xattr -d com.apple.quarantine ./snap` once.

## API

| Function | Description |
|----------|-------------|
| `snap.read(path)` | Load a file as a queryable view (parquet/csv/json) or attach a database (.db/.sqlite) |
| `snap.meta()` | Print schema metadata for all loaded tables and views |
| `snap.query(sql, format?)` | Execute SQL and display results. Format: `"table"`, `"markdown"`, `"json"` |
| `snap.profile(target)` | Run statistical profiling (min, max, avg, std, quantiles) on a table/view |
| `snap.export(path, sql_or_table)` | Export query results or a table to a file (format inferred from extension) |

## Example Pipeline

```lua
-- Load and analyze employee data
snap.read("employees.parquet")

-- Explore the schema
snap.meta()

-- Run analytics
snap.query([[
    SELECT department, COUNT(*) as headcount, ROUND(AVG(salary), 2) as avg_salary
    FROM employees
    GROUP BY department
    ORDER BY avg_salary DESC
]], "markdown")

-- Profile the data
snap.profile("employees")

-- Export results
snap.export("summary.csv", "SELECT * FROM employees WHERE salary > 100000")
```

## Building

Requirements (macOS via Homebrew):

```bash
brew install lua duckdb sqlite
make
```

Run tests:

```bash
make test
```

## Architecture

```
┌─────────────────────────────────┐
│        Lua 5.4 Runtime          │
├─────────────────────────────────┤
│       snap C bindings           │
├────────────────┬────────────────┤
│    DuckDB      │    SQLite3     │
│  (analytics)   │   (auditing)   │
└────────────────┴────────────────┘
```

- **DuckDB** handles all analytical workloads: columnar scans, parquet/csv ingestion, SQL execution
- **SQLite3** provides lightweight query history and metadata auditing
- **Lua** serves as the scripting and orchestration layer

## License

MIT
