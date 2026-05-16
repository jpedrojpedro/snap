CC = cc
CFLAGS = -O2 -Wall \
	-I/opt/homebrew/include/lua \
	-I/opt/homebrew/opt/sqlite/include \
	-I/opt/homebrew/opt/duckdb/include

SRCS = src/main.c src/lua_dw.c
TARGET = snap

DUCKDB_LIBS = /opt/homebrew/opt/duckdb/lib
STATIC_LIBS = \
	/opt/homebrew/opt/lua/lib/liblua.a \
	/opt/homebrew/opt/sqlite/lib/libsqlite3.a \
	$(DUCKDB_LIBS)/libduckdb_static.a \
	$(DUCKDB_LIBS)/libparquet_extension.a \
	$(DUCKDB_LIBS)/libjson_extension.a \
	$(DUCKDB_LIBS)/libicu_extension.a \
	$(DUCKDB_LIBS)/libcore_functions_extension.a \
	$(DUCKDB_LIBS)/libautocomplete_extension.a \
	$(DUCKDB_LIBS)/libduckdb_fastpforlib.a \
	$(DUCKDB_LIBS)/libduckdb_fmt.a \
	$(DUCKDB_LIBS)/libduckdb_fsst.a \
	$(DUCKDB_LIBS)/libduckdb_hyperloglog.a \
	$(DUCKDB_LIBS)/libduckdb_mbedtls.a \
	$(DUCKDB_LIBS)/libduckdb_miniz.a \
	$(DUCKDB_LIBS)/libduckdb_pg_query.a \
	$(DUCKDB_LIBS)/libduckdb_re2.a \
	$(DUCKDB_LIBS)/libduckdb_skiplistlib.a \
	$(DUCKDB_LIBS)/libduckdb_utf8proc.a \
	$(DUCKDB_LIBS)/libduckdb_yyjson.a \
	$(DUCKDB_LIBS)/libduckdb_zstd.a \
	$(DUCKDB_LIBS)/libduckdb_generated_extension_loader.a

LDFLAGS = -lc++ -lm

all: $(TARGET)

$(TARGET): $(SRCS) src/lua_dw.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(STATIC_LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET) tests/mock_staff.parquet tests/high_earners.csv

test: $(TARGET)
	./$(TARGET) tests/test_pipeline.lua

.PHONY: all clean test
