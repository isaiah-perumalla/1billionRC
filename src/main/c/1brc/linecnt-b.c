//
// Created by isaiahp on 19/10/2025.
//
#define _GNU_SOURCE // O_DIRECT flag not defined without this

#include <stdio.h>
#include <stdlib.h>
#include <liburing.h>
#include <assert.h>
#include <fcntl.h>
#include "include/asyncio.h"

#define BLK_SIZE (1024 * 256) // 256k block seems optimal
#define IS_POW2(x) (0 == ((x) & ((x) - 1)))
const int BUFF_GRP_ID = 1337;


__uint64_t total;

inline __u8 count_lf(const __u64 bytes) {
    assert('\n'== 0x0A);
    const __u64 new_line_mask = 0x0A0A0A0A0A0A0A0A;
    __u64 tmp = (bytes ^ new_line_mask); //zero bytes at new line position
    tmp = (tmp & 0x7f7f7f7f7f7f7f7f) + 0x7f7f7f7f7f7f7f7f; //every 0 byte and 0x80 will have 7F
    //need to replace 0 byte with 0x80 and all other bytes with 0
    tmp = ~(tmp | bytes |  0x7f7f7f7f7f7f7f7f);
    tmp = (tmp >> 7); //every zero byte will be a 0x01
    return tmp % 0xFF; //count number of 1 bytes in work
}

static __u64 count_new_lines(const char *data, const __uint64_t size) {
    const __u8 chunk = 64;
    __u8 remainder = size & (chunk - 1);
    __uint64_t count = 0;
    const __uint64_t length = (size - remainder);
    assert((length & (chunk -1)) == 0); // multiple of chuck
    __uint64_t i;
    for ( i = 0; i < length; i+=chunk) {
        count += count_lf(*(__u64 *) (data + i));
        count += count_lf(*(__u64 *) (data + i + 8));
        count += count_lf(*(__u64 *) (data + i + 16));
        count += count_lf(*(__u64 *) (data + i + 24));
        count += count_lf(*(__u64 *) (data + i + 32));
        count += count_lf(*(__u64 *) (data + i + 40));
        count += count_lf(*(__u64 *) (data + i + 48));
        count += count_lf(*(__u64 *) (data + i + 56));
    }
    while (remainder-- > 0) {
        if (data[i++] == '\n') {
            count += 1;
        }
    }
    return count;
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }
    const int fd = open(argv[1], O_RDONLY | O_DIRECT);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct stat finfo;
    if (fstat(fd, &finfo)) {
        perror("fstat");
        exit(1);
    }
    struct async_reader_t reader = async_reader_new( BLK_SIZE);

    assert(finfo.st_size >= 0);
    const __u64 file_size = finfo.st_size;
    //setup io ring

    int err = async_reader_init(&reader, fd, 0, file_size);
    if (err) {
        fprintf(stderr,"async read init failed for fd=%d", fd);
        exit(1);
    }
    //submit read requests


    __uint64_t processed_bytes = 0;
    __uint64_t lines = 0;

    while (processed_bytes < file_size) {

        int poll = async_reader_poll(&reader);

        if (poll < 0) { //fatal error
            fprintf(stderr, "io_uring_peek_cqe returned %d\n", poll);
            exit(1);
        }
        __uint64_t bytes_read = 0;
        const char *data = async_reader_next_ready(&reader, &bytes_read);
        if (bytes_read == 0) {
            continue; //blk not ready
        }
        if (processed_bytes + BLK_SIZE <= file_size) {
            assert(bytes_read == BLK_SIZE);
        }
        lines += count_new_lines(data, bytes_read);

        processed_bytes += bytes_read;
        async_reader_advance_read(&reader, bytes_read);

    }
    printf("lines %ld\n", lines);
    printf("bytes read  %ld\n", processed_bytes);
    clock_gettime(CLOCK_MONOTONIC, &end);

    const __uint64_t seconds = end.tv_sec - start.tv_sec;
    const __uint64_t nanos = end.tv_nsec - start.tv_nsec;
    const float elapsed = (float)(seconds * 1000000000 + nanos);
    printf("time: %.3f millis\n", elapsed/1000000);
    return 0;
}
