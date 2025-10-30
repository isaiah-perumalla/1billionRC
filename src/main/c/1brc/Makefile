CC=gcc
CCC=clang
LIBURING_BUILD=../../../../local-build
DEBUG_FLAGS=-g -fno-omit-frame-pointer
RELEASE_FLAGS=-DNDEBUG
CFLAGS=-O3 -Wall -g -Wextra -I $(LIBURING_BUILD)/include -Iinclude
ODIR=build
LIBS=$(LIBURING_BUILD)/lib/liburing.a
BIN=bin
SRC=.
DEPS=$(BIN)/asyncio_gcc.o
all: $(BIN)/linecnt_gcc $(BIN)/linecnt_clang $(BIN)/linecnt-a_gcc $(BIN)/linecnt-a_clang $(BIN)/linecnt-b_gcc $(BIN)/linecnt-b_clang $(BIN)/baseline_linecnt_gcc $(BIN)/baseline_linecnt_clang $(BIN)/baseline_read_gcc $(BIN)/baseline_read_clang

$(BIN):
	mkdir -p $(BIN)


$(BIN)/%_gcc: $(SRC)/%.c $(BIN)/asyncio_gcc.o
	$(CC) $<   -o $@ $(BIN)/asyncio_gcc.o $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)

$(BIN)/%_clang: $(SRC)/%.c $(BIN)/asyncio_clang.o
	$(CC) $<   -o $@ $(BIN)/asyncio_clang.o $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)

$(BIN)/asyncio_gcc.o:
	$(CC)  -c $(CFLAGS) asyncio.c -o $@

$(BIN)/asyncio_clang.o:
	$(CC)  -c $(CFLAGS) asyncio.c -o $@


$(BIN)/asyncio_gcc: $(SRC)/asyncio.c
	$(CC) $(CFLAGS) $(SRC)/asyncio.c -o $(BIN)/asyncio_gcc.o   $(LIBS)

.PHONY: clean

clean:
	rm -rf $(BIN)/*
