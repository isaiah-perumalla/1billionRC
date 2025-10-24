#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define  PAGE_SIZE  (1024*64)
#define BLOCKSIZE 512
int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }
    const char *filename = argv[1];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    const int fd = open(filename, O_RDONLY, 0);
    if (fd == -1) {
        perror("fopen");
        exit(1);
    }
    const int err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (err) {
        perror("posix_fadvise");
        exit(1);
    }
    size_t n = 0;
    void *buf;
    posix_memalign(&buf, BLOCKSIZE, PAGE_SIZE);
    size_t total = 0;
    __uint64_t chkSum = 0;
    while ((n = read(fd, buf, PAGE_SIZE)) > 0) {
        total += n;
        if (n > 7) {
            const __uint64_t* val = (__uint64_t*)buf;
            chkSum += *val;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("chkSum: %ld\n", chkSum);
    printf("total-bytes: %ld\n", total);
    const __uint64_t seconds = end.tv_sec - start.tv_sec;
    const __uint64_t nanos = end.tv_nsec - start.tv_nsec;
    const float elapsed = (float)(seconds * 1000000000 + nanos);
    printf("time: %.3f millis\n", elapsed/1000000);
    return 0;
}
