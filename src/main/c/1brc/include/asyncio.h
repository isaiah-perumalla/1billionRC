//
// Created by isaiahp on 27/10/2025.
//

#ifndef INC_ASYNCIO_H
#define INC__ASYNCIO_H



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

/**
 * create a new async reader
 * @param blk_size
 * @return
 */
struct async_reader_t async_reader_new(__u32 blk_size);

// init reading of fd from offset to size
int async_reader_init(struct async_reader_t *reader, int fd, __uint64_t offset, __uint64_t size);

int async_reader_poll(struct async_reader_t *reader);

const char* async_reader_next_ready(struct async_reader_t *reader, __uint64_t* size);

__u64 async_reader_advance_read(struct async_reader_t *r, const __uint64_t size);

#endif