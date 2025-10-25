#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <asm-generic/int-ll64.h>

int PAGE_SIZE = 8912;

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }
    const char *filename = argv[1];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    FILE *fh = fopen(filename, "r");
    if (!fh) {
        perror("error opening file");
        exit(EXIT_FAILURE);
    }

    size_t n = 0;
    char buf[PAGE_SIZE];
    size_t total = 0;
    __u64 lines = 0;
    while ((n = fread(buf, 1, PAGE_SIZE, fh)) > 0) {
        total += n;
        for (__uint64_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                lines++;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("total: %lld\n", lines);
    const __uint64_t seconds = end.tv_sec - start.tv_sec;
    const __uint64_t nanos = end.tv_nsec - start.tv_nsec;
    const float elapsed = (float)(seconds * 1000000000 + nanos);
    printf("time: %.3f millis\n", elapsed/1000000);

    return 0;
}
