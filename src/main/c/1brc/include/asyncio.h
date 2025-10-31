//
// Created by isaiahp on 27/10/2025.
//

#ifndef INC_ASYNCIO_H
#define INC_ASYNCIO_H


/**
 * create a new async reader
 * @param blk_size
 * @return
 */
struct async_reader_t* async_reader_new(__u32 blk_size);

// init reading of fd from offset to size
int async_reader_init(struct async_reader_t *reader, int fd, __uint64_t offset, __uint64_t size);

/**
 * clean up resources used by reader
 * @param reader
 * @return
 */
int async_reader_free(struct async_reader_t *reader);

/**
 *
 * @param reader
 * @return number of block requests completed and received from kernel
 */
int async_reader_poll(struct async_reader_t *reader);

/**
 * blocks can be completed out of order,
 * this method will ensure block are return in order
 * return the next block that ready to be read
 * @param reader
 * @param size
 * @return ptr to buffer with data for next block that is ready for reading , NULL and size == 0 if next block in order is not yet ready
 */
const char* async_reader_next_ready(struct async_reader_t *reader, __uint64_t* size);

/**
 * Notify reader blk is consumed, notifies kernel to reuse the block for subsequent reads 
 * @param r
 * @param size
 * @return
 */
__u64 async_reader_advance_read(struct async_reader_t *r);

#endif