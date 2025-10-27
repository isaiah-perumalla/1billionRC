//
// Created by isaiahp on 19/10/2025.
//
#define _GNU_SOURCE // O_DIRECT flag not defined without this

#include <stdio.h>
#include <stdlib.h>
#include <liburing.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>

#define BLK_SIZE (1024 * 256) // 256k block seems optimal
#define NR_BUFS 64
#define BR_MASK (NR_BUFS-1)
#define IS_POW2(x) (0 == ((x) & ((x) - 1)))
const int BUFF_GRP_ID = 1337;


__uint64_t total;


//keep track of which block to read next
struct blk_read_stat {
    __u64 next_offset;
    __u64 submitted_offset;
    __u64 length;
    //track which buffers are ready to be read
    __u16 ready_buffers[NR_BUFS];
    __u8 ready_count;
};


static bool prep_read_blk(struct io_uring* ring_ptr, const int fd, const unsigned nbytes, const __u64 file_offset) {
    assert(nbytes > 0);
    struct io_uring_sqe * sqe = io_uring_get_sqe(ring_ptr);
    if (sqe == NULL) { // no more entries can be submitted
        return false;
    }
    io_uring_prep_read(sqe, fd, NULL, nbytes, file_offset);
    sqe->buf_group = BUFF_GRP_ID;
    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->user_data = file_offset;
    return true;
}

static void init_blk_read(struct blk_read_stat * blk_read_stat, __u64 submitted_size, __u64 fsize) {
    assert(submitted_size <= fsize);
    memset(blk_read_stat->ready_buffers, 0, sizeof(__u16) * NR_BUFS);
    blk_read_stat->next_offset = 0;
    blk_read_stat->ready_count = 0;
    blk_read_stat->submitted_offset = submitted_size;
    blk_read_stat->length = fsize;
}

//record a completion of blk request
static void accept_cqe(struct blk_read_stat * blk_read_stat, const struct io_uring_cqe * cqe) {
    const __u16 bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
    //process bytes
    //user_date is file_offset in to file
    const __u64 data_offset = cqe->user_data;
    const __u64 size = cqe->res;
    assert(bid > 0 && bid <= NR_BUFS); // bid start from 1
    assert(size > 0 && size <= BLK_SIZE);
    assert(data_offset >= blk_read_stat->next_offset);
    assert(data_offset < blk_read_stat->submitted_offset);
    //size < BLK_SIZE -> last block
    assert(size == BLK_SIZE || data_offset + size == blk_read_stat->length);
    const __u64 blk_nr = data_offset / BLK_SIZE;
    const __u16 idx = blk_nr & BR_MASK;
    assert(idx < NR_BUFS);
    assert(blk_read_stat->ready_buffers[idx] == 0);
    blk_read_stat->ready_buffers[idx] = bid;
    blk_read_stat->ready_count += 1;
}

//block id for next read
static  __u16 next_read_bid(struct blk_read_stat * blk_read_stat) {
    const __u64 blk_nr = blk_read_stat->next_offset / BLK_SIZE;
    const __u16 idx = blk_nr & BR_MASK;
    const __u16 bid = blk_read_stat->ready_buffers[idx];
    return bid;
}

static void blk_stat_advance_read(struct blk_read_stat * blk_read_stat, __uint64_t bytes_read) {
    assert(blk_read_stat->next_offset + bytes_read <= blk_read_stat->submitted_offset);
    assert(blk_read_stat->next_offset + bytes_read <= blk_read_stat->length);
    const __u64 blk_nr = blk_read_stat->next_offset / BLK_SIZE;
    const __u16 idx = blk_nr & BR_MASK;
    blk_read_stat->ready_buffers[idx] = 0; //mark as empty
    blk_read_stat->next_offset += bytes_read;
    blk_read_stat->ready_count -= 1;
}

static void blk_stat_advance_submitted(struct blk_read_stat * blk_read_stat, __u64 nbytes) {
    assert(blk_read_stat->submitted_offset + nbytes <= blk_read_stat->length);
    //cannot have more than NR_BUFF  buffer inflight
    assert((nbytes + blk_read_stat->submitted_offset - blk_read_stat->next_offset)  <= BLK_SIZE * NR_BUFS);
    blk_read_stat->submitted_offset += nbytes;
}

static __u64 blk_stat_next_blk_size(struct blk_read_stat * blk_read_stat) {
    const __u64 remaining = blk_read_stat->length - blk_read_stat->submitted_offset;
    return remaining < BLK_SIZE ? remaining : BLK_SIZE;
}

static bool check_cqe(int peek_res, struct io_uring_cqe * cqe) {

    if (peek_res == 0) {
        if (!(cqe->flags & IORING_CQE_F_BUFFER)) {
            fprintf(stderr, "no buffer selected\n");
            exit(1);
        }
        if (cqe->res <= 0) {
            if (cqe->res == -ENOBUFS) {
                fprintf(stderr, "no buffers available \n");
            }
            if (cqe->res == 0) {
                fprintf(stderr, "cqe res zero, offset=%lld \n", cqe->user_data);
            }
            fprintf(stderr,"Failed cqe result %d \n", cqe->res);
            exit(1);
        }
        return true;
    }
    if (peek_res == -EAGAIN) { //no pending work
        return false;
    }
    //error cases

    fprintf(stderr, "io_uring_peek_cqe returned %d\n", peek_res);
    exit(1);
}

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

    //ensure NR_BUFF is pow of 2 or break all assumptions in rest of code
    assert(IS_POW2(NR_BUFS));
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    struct io_uring ring;
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
    //pre-allocate buffers
    //with page alignment
    char* buffers = aligned_alloc(4096, BLK_SIZE * NR_BUFS );

    assert(finfo.st_size >= 0);
    const __u64 file_size = finfo.st_size;
    //setup io ring
    const int ret = io_uring_queue_init(NR_BUFS, &ring, 0);
    if (ret) {
        fprintf(stderr, "io_uring_queue_init check kernel version > 5.19");
        exit(1);
    }
    //setup buffer ring
    int result;
    struct io_uring_buf_ring* buf_ring_ptr;
    buf_ring_ptr = io_uring_setup_buf_ring(&ring, NR_BUFS, BUFF_GRP_ID, 0, &result);
    if (buf_ring_ptr == NULL) {
        fprintf(stderr,"io_uring_setup_buf_ring, check kernel version > 5.19");
        exit(1);
    }
    void * ptr = buffers;
    //register the memory into the buffer pool
    //kernel can reuse this avoid copies to userspace
    //bid -> buffer-id, bid 0 is not used
    for (int i = 0; i < NR_BUFS; i++) {
        io_uring_buf_ring_add(buf_ring_ptr, ptr, BLK_SIZE, i+1, BR_MASK, i);
        ptr += BLK_SIZE;
    }
    io_uring_buf_ring_advance(buf_ring_ptr, NR_BUFS);
    __u64 submitted_bytes = 0;
    //submit read requests
    int n;

    for ( n = 0; n < NR_BUFS && submitted_bytes < file_size; n++) {
        const __u64 remaining_bytes = (file_size - submitted_bytes);
        const __u64 nbytes = remaining_bytes >= BLK_SIZE ? BLK_SIZE : remaining_bytes;
        const __u64 file_offset = n * BLK_SIZE;
        prep_read_blk(&ring, fd, BLK_SIZE, file_offset); //issue BLK_SIZE worth of reads but app know to consume nbytes
        submitted_bytes += nbytes;
    }
    const int submit_result = io_uring_submit(&ring);
    if (submit_result != n) {
        fprintf(stderr, "submit-expected: %d but was %d\n", submit_result, n);
        return 1;
    }

    struct blk_read_stat blk_read_stat;
    init_blk_read(&blk_read_stat, submitted_bytes, file_size);

    __uint64_t processed_bytes = 0;
    __uint64_t lines = 0;

    while (processed_bytes < file_size) {
        struct io_uring_cqe * cqe;

        int peek_res = io_uring_peek_cqe(&ring, &cqe);

        const bool new_blk = check_cqe(peek_res, cqe);

        if (new_blk) {
            accept_cqe(&blk_read_stat, cqe);
            io_uring_cqe_seen(&ring, cqe); // mark as seen
        }
        const __u16 next_bid = next_read_bid(&blk_read_stat);
        if (next_bid == 0) {
            continue; //blk not ready
        }

        //data matches expected offset
        const char *data = buffers + (BLK_SIZE * (next_bid -1));
        //always BLK_SIZE of data read unless it is last block
        const __uint64_t bytes_read = (processed_bytes + BLK_SIZE) <= file_size ?  BLK_SIZE : (file_size - processed_bytes);

        lines += count_new_lines(data, bytes_read);

        processed_bytes += bytes_read;
        blk_stat_advance_read(&blk_read_stat, bytes_read);
        //advance both buffer ring and cqe ring
        io_uring_buf_ring_advance(buf_ring_ptr, 1);// release buffer back to kernel
        const __u64 nbytes = blk_stat_next_blk_size(&blk_read_stat);
        //always submit blk_size reads even if it larger than file size, n_bytes keeps track of how much is read
        // O_DIRECT reads need to be issues at multiple BLK_SIZE
        if (nbytes > 0 && prep_read_blk(&ring, fd, BLK_SIZE, blk_read_stat.submitted_offset)) {
            result = io_uring_submit(&ring);
            assert(result == 1);
            blk_stat_advance_submitted(&blk_read_stat, nbytes); // advance
        }
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
