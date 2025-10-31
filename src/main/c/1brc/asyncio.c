//
// Created by isaiahp on 27/10/2025.
//
#define _GNU_SOURCE // O_DIRECT flag not defined without this
#include <stdio.h>
#include <stdlib.h>
#include <liburing.h>
#include <assert.h>
#include <string.h>
#include "include/asyncio.h"

#define NR_BUFF 64

#define IS_POW2(x) (0 == ((x) & ((x) - 1)))

#define NEW_INSTANCE(t)  (t*)malloc(sizeof(t))
#define FREE(p) free((p)), (p) = NULL

static const int BUFF_GRP_ID = 1337;

struct async_reader_t {
    struct io_uring ring;
    struct io_uring_buf_ring *buf_ring_ptr;
    char * buffers;
    __u32 blk_size;
    __u64 next_offset;
    __u64 submitted_offset;
    __u64 length;
    //track which buffers are ready to be read
    __u16 ready_buffers[64];
    __u8 ready_count;
    int fd;
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


int async_reader_free(struct async_reader_t *reader) {
    io_uring_unregister_buf_ring(&reader->ring, BUFF_GRP_ID);
    io_uring_queue_exit(&reader->ring);
    FREE(reader->buffers);
    FREE(reader);
    return 0;
}

struct async_reader_t* async_reader_new(__u32 blk_size) {
    assert(IS_POW2(blk_size));//pow2
    //pre-allocate buffers
    //with page alignment
    char* buffers = aligned_alloc(4096, blk_size * NR_BUFF);
    struct async_reader_t* t = NEW_INSTANCE(struct async_reader_t);
    t->buffers = buffers;
    t->blk_size = blk_size;
    t->submitted_offset = 0;
    t->length = 0;
    t->next_offset = 0;
    t->ready_count = 0;
    t->fd = -1; //not init
    const int ret = io_uring_queue_init(NR_BUFF, &t->ring, 0);
    if (ret) {
        fprintf(stderr, "io_uring_queue_init check kernel version > 5.19");
        exit(1);
    }
    //setup buffer ring
    int result;
    t->buf_ring_ptr = io_uring_setup_buf_ring(&t->ring, NR_BUFF, BUFF_GRP_ID, 0, &result);
    if (t->buf_ring_ptr == NULL) {
        fprintf(stderr,"io_uring_setup_buf_ring, check kernel version > 5.19");
        exit(1);
    }
    void * ptr = buffers;
    //register the memory into the buffer pool
    //kernel can reuse this avoid copies to userspace
    //bid -> buffer-id, bid 0 is not used
    for (int i = 0; i < NR_BUFF; i++) {
        io_uring_buf_ring_add(t->buf_ring_ptr, ptr, blk_size, i+1, NR_BUFF-1, i);
        ptr += blk_size;
    }
    io_uring_buf_ring_advance(t->buf_ring_ptr, NR_BUFF);
    return t;
}

// init reading of fd from offset to size
int async_reader_init(struct async_reader_t *t, int fd, __uint64_t offset, __uint64_t size) {
    if (size <= 0) return -1;
    assert((offset & (t->blk_size -1)) == 0); //offset is multiple of block size
    //submit read requests

    int n;
    t->fd = fd;
    __uint64_t submitted_bytes = 0;
    for ( n = 0; n < NR_BUFF && submitted_bytes < size; n++) {
        const __u64 remaining_bytes = (size - submitted_bytes);
        const __u64 nbytes = remaining_bytes >= t->blk_size ? t->blk_size : remaining_bytes;
        const __u64 file_offset = offset + (n * t->blk_size);
        prep_read_blk(&t->ring, fd, t->blk_size, file_offset); //issue BLK_SIZE worth of reads but app know to consume nbytes
        submitted_bytes += nbytes;
    }
    const int submit_result = io_uring_submit(&t->ring);
    if (submit_result != n) {
        fprintf(stderr, "submit-expected: %d but was %d\n", submit_result, n);
        return -1;
    }
    assert(submitted_bytes <= size);
    memset(t->ready_buffers, 0, sizeof(__u16) * NR_BUFF);
    t->next_offset = offset;
    t->ready_count = 0;
    t->submitted_offset = offset + submitted_bytes;
    t->length = offset + size;

    return 0;
}

static int check_cqe(int peek_res, struct io_uring_cqe * cqe) {

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
        return 1;
    }
    if (peek_res == -EAGAIN) { //no pending work
        return 0;
    }
    //error cases

    assert(peek_res < 0); // ensure error case
    return peek_res;
}

//record a completion of blk request
static void accept_cqe(struct async_reader_t * t, const struct io_uring_cqe * cqe) {
    const __u16 bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
    //process bytes
    //user_date is file_offset in to file
    const __u64 data_offset = cqe->user_data;
    assert(bid > 0 && bid <= NR_BUFF); // bid start from 1
    assert(cqe->res > 0 && ((__u32)cqe->res) <= t->blk_size);
    assert(data_offset >= t->next_offset);
    assert(data_offset < t->submitted_offset);
    //size < BLK_SIZE -> last block
    assert((__u32)cqe->res == t->blk_size || data_offset + cqe->res == t->length);
    const __u64 blk_nr = data_offset / t->blk_size;
    const __u16 idx = blk_nr & (NR_BUFF -1);
    assert(idx < NR_BUFF);
    assert(t->ready_buffers[idx] == 0);
    t->ready_buffers[idx] = bid;
    t->ready_count += 1;
}

int async_reader_poll(struct async_reader_t *reader) {
    struct io_uring_cqe * cqe;

    int peek_res = io_uring_peek_cqe(&reader->ring, &cqe);
    const int new_blk = check_cqe(peek_res, cqe);
    if (new_blk == 1) {
        accept_cqe(reader, cqe);
        io_uring_cqe_seen(&reader->ring, cqe); // mark as seen
    }
    return new_blk;
}

const char* async_reader_next_ready(struct async_reader_t *reader, __uint64_t* size) {
    const __u32 blk_size = reader->blk_size;

    //data matches expected offset
    const __u64 blk_nr = reader->next_offset / blk_size;
    const __u16 idx = blk_nr & (NR_BUFF - 1);
    const __u16 bid = reader->ready_buffers[idx];
    if (bid == 0) { //awaiting next blk
        *size = 0;
        return NULL;
    }

    const char *data = reader->buffers + (blk_size * (bid -1));
    //always BLK_SIZE of data read unless it is last block
    const __uint64_t bytes_read = (reader->next_offset + blk_size) <= reader->length ?  blk_size : (reader->length - reader->next_offset);
    *size = bytes_read;
    return data;

}


__u64 async_reader_advance_read(struct async_reader_t *r) {
    const __u32 blk_size = r->blk_size;
    const __u64 blk_nr = r->next_offset / blk_size;
    const __u16 idx = blk_nr & (NR_BUFF-1);
    r->ready_buffers[idx] = 0; //mark as empty
    if (blk_size + r->next_offset <= r->length) {
        r->next_offset += blk_size;
    }
    else {
        assert(r->length == r->submitted_offset);
        r->next_offset = r->length;
    }
    r->ready_count -= 1;//advance both buffer ring and cqe ring
    io_uring_buf_ring_advance(r->buf_ring_ptr, 1);// release buffer back to kernel

    const __u64 remaining = r->length - r->submitted_offset;
    const __u64 nbytes = remaining < blk_size ? remaining : blk_size;

    //always submit blk_size reads even if it larger than file size, n_bytes keeps track of how much is read
    // O_DIRECT reads need to be issues at multiple blk_size
    if (nbytes > 0 && prep_read_blk(&r->ring, r->fd, blk_size, r->submitted_offset)) {
        const int result = io_uring_submit(&r->ring);
        if (result != 1) {
            return 0; //err
        }
        assert(result == 1);
        assert(r->submitted_offset + nbytes <= r->length);
        //cannot have more than NR_BUFF  buffer inflight
        assert((nbytes + r->submitted_offset - r->next_offset)  <= r->blk_size * NR_BUFF);
        r->submitted_offset += nbytes;
    }
    return r->next_offset;
}

