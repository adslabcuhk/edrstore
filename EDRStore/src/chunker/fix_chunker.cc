/**
 * @file fix_chunker.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of FixChunker
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/chunker/fix_chunker.h"

/**
 * @brief Construct a new Fix Chunker object
 * 
 */
FixChunker::FixChunker() {
    tool::Logging(my_name_.c_str(), "init FixChunker.\n");
}

/**
 * @brief Destroy the Fix Chunker object
 * 
 */
FixChunker::~FixChunker() {

}

/**
 * @brief load the data from the file
 * 
 * @param input_file the input file handler
 * @return uint32_t the read size
 */
uint32_t FixChunker::LoadDataFromFile(ifstream& input_file) {
    input_file.read((char*)read_data_buf_, read_size_);
    pending_chunking_size_ = input_file.gcount();

    // reset the offset
    cur_offset_ = 0;
    remain_chunking_size_ = pending_chunking_size_;

    return pending_chunking_size_;
}

/**
 * @brief generate a chunk
 * 
 * @param data the buffer to store the chunk data
 * @return uint32_t the chunk size
 */
uint32_t FixChunker::GenerateOneChunk(uint8_t* data) {
    if (pending_chunking_size_ == 0 || remain_chunking_size_ == 0) {
        return 0; // this is end of the pending buffer
    }
    
    uint32_t chunk_size = 0;
    if (remain_chunking_size_ >= avg_chunk_size_) {
        memcpy(data, read_data_buf_ + cur_offset_, avg_chunk_size_);
        chunk_size = avg_chunk_size_;
    } else {
        memcpy(data, read_data_buf_ + cur_offset_, remain_chunking_size_);
        chunk_size = remain_chunking_size_;        
    }

    cur_offset_ += chunk_size;
    remain_chunking_size_ -= chunk_size;

    _total_chunk_num++;
    _total_file_size += chunk_size;
    return chunk_size;
}