CC=gcc
LIBURING_BUILD=../../../../local-build
CFLAGS=-O2 -Wall -Wextra -g -I $(LIBURING_BUILD)/include
ODIR=build
LIBS=$(LIBURING_BUILD)/lib/liburing.a

all: read_bench fread_bench iouring_read_bench

read_bench: read_bench.o
	$(CC)  $(CFLAGS) -o $@  read_bench.o

fread_bench: fread_bench.o
	$(CC) $(CFLAGS) -o $@ fread_bench.o


iouring_read_bench: iouring_read_bench.o
	$(CC)  $(CFLAGS)  -o $@ iouring_read_bench.o  $(LIBS)

.PHONY: clean

clean:
	rm -f *.o read_bench  fread_bench iouring_read_bench