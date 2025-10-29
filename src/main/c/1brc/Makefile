CC=gcc
LIBURING_BUILD=../../../../local-build
DEBUG_FLAGS=-g -fno-omit-frame-pointer
CFLAGS=-O2 -Wall -g -Wextra -I $(LIBURING_BUILD)/include -Iinclude
ODIR=build
LIBS=$(LIBURING_BUILD)/lib/liburing.a

all: linecnt linecnt-a read_bench baseline_linecnt

read_bench: read_bench.o
	$(CC)  $(CFLAGS) -o $@  read_bench.o

baseline_linecnt: baseline_linecnt.o
	$(CC) $(CFLAGS) -o $@ baseline_linecnt.o



linecnt: asyncio.o linecnt.o
	$(CC) $(CFLAGS)  -o $@ asyncio.o linecnt.o  $(LIBS)

linecnt-a: asyncio.o linecnt-a.o
	$(CC) $(CFLAGS)  -o $@ asyncio.o linecnt-a.o  $(LIBS)

linecnt-b: asyncio.o linecnt-b.o
	$(CC) $(CFLAGS)  -o $@ asyncio.o linecnt-b.o  $(LIBS)

.PHONY: clean

clean:
	rm -f *.o read_bench baseline_linecnt linecnt linecnt-a linecnt-b