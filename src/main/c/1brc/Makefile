CC=gcc
CCC=clang
LIBURING_BUILD=../../../../local-build
DEBUG_FLAGS=-g -fno-omit-frame-pointer
RELEASE_FLAGS=-O3 -DNDEBUG
CFLAGS=-Wall -g -Wextra -I $(LIBURING_BUILD)/include -Iinclude
ODIR=build
LIBS=$(LIBURING_BUILD)/lib/liburing.a
BIN=bin
SRC=.
DEPS=$(BIN)/asyncio_gcc.o
all: $(BIN)/linecnt_gcc $(BIN)/linecnt_clang $(BIN)/linecnt-a_gcc $(BIN)/linecnt-a_clang $(BIN)/linecnt-b_gcc $(BIN)/linecnt-b_clang \
$(BIN)/baseline_linecnt_gcc $(BIN)/baseline_linecnt_clang $(BIN)/baseline_read_gcc $(BIN)/baseline_read_clang \
$(BIN)/linecnt_debug_gcc $(BIN)/linecnt-a_debug_gcc $(BIN)/linecnt-b_debug_gcc

$(BIN):
	mkdir -p $(BIN)

$(BIN)/%_debug_gcc: $(SRC)/%.c $(BIN)/asyncio_debug_gcc.o
	$(CC) $<   -o $@ $(BIN)/asyncio_debug_gcc.o $(CFLAGS) $(DEBUG_FLAGS) $(LIBS)


$(BIN)/%_gcc: $(SRC)/%.c $(BIN)/asyncio_gcc.o
	$(CC) $<   -o $@ $(BIN)/asyncio_gcc.o $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)

$(BIN)/%_clang: $(SRC)/%.c $(BIN)/asyncio_clang.o
	$(CC) $<   -o $@ $(BIN)/asyncio_clang.o $(CFLAGS) $(RELEASE_FLAGS) $(LIBS)


$(BIN)/asyncio_debug_gcc.o: $(BIN)
	$(CC)  -c $(CFLAGS) $(DEBUG_FLAGS)  asyncio.c -o $@

$(BIN)/asyncio_gcc.o: $(BIN)
	$(CC)  -c $(CFLAGS) $(RELEASE_FLAGS) asyncio.c -o $@

$(BIN)/asyncio_clang.o: $(BIN)
	$(CC)  -c $(CFLAGS) $(RELEASE_FLAGS) asyncio.c -o $@



.PHONY: clean

clean:
	rm -rf $(BIN)/*
