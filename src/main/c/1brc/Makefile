CC=gcc
LIBURING_BUILD=../../../../local-build
CFLAGS=-O2 -Wall -Wextra -g -I $(LIBURING_BUILD)/include
ODIR=build
LIBS=$(LIBURING_BUILD)/lib/liburing.a

all: linecnt read_bench baseline_linecnt

read_bench: read_bench.o
	$(CC)  $(CFLAGS) -o $@  read_bench.o

baseline_linecnt: baseline_linecnt.o
	$(CC) $(CFLAGS) -o $@ baseline_linecnt.o


linecnt: linecnt.o
	$(CC)  $(CFLAGS)  -o $@ linecnt.o  $(LIBS)

.PHONY: clean

clean:
	rm -f *.o read_bench baseline_linecnt linecnt