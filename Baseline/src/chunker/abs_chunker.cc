/**
 * @file abs_chunker.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface 
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/chunker/abs_chunker.h"

/**
 * @brief Construct a new Abs Chunker object
 * 
 */
AbsChunker::AbsChunker() {
    avg_chunk_size_ = config.GetAvgChunkSize();
    min_chunk_size_ = config.GetMinChunkSize();
    max_chunk_size_ = config.GetMaxChunkSize();
    read_size_ = config.GetReadSize();
    read_size_ *= (1 << 20);
    read_data_buf_ = (uint8_t*) malloc(read_size_ * sizeof(uint8_t));
}

/**
 * @brief Destroy the Abs Chunker object
 * 
 */
AbsChunker::~AbsChunker() {
    fprintf(stderr, "========AbsChunker Info========\n");
    fprintf(stderr, "total file size: %lu\n", _total_file_size);
    fprintf(stderr, "total chunk num: %lu\n", _total_chunk_num);
    fprintf(stderr, "===============================\n");
    free(read_data_buf_);
}