CC=gcc
CFLAGS=-Wall -Wextra -g
ODIR=build

all: read_bench fread_bench

read_bench: read_bench.o
	$(CC) $(CFLAGS) -o $@  read_bench.o $(LIBS)

fread_bench: fread_bench.o
	$(CC) $(CFLAGS) -o $@ fread_bench.o  $(LIBS)


.PHONY: clean

clean:
	rm -f *.o read_bench fread_bench