/**
 * @file abs_chunker.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the interface of abs chunker
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_ABS_CHUNKER_H
#define MY_CODEBASE_ABS_CHUNKER_H

#include "../define.h"
#include "../configure.h"

using namespace std;

extern Configure config;

class AbsChunker {
    protected:
        string my_name_ = "AbsChunker";

        // chunk size setting
        uint64_t min_chunk_size_ = 0;
        uint64_t avg_chunk_size_ = 0;
        uint64_t max_chunk_size_ = 0;
        
        // buffer
        uint64_t read_size_ = 0;
        uint8_t* read_data_buf_;

        // size
        uint64_t pending_chunking_size_ = 0;
        uint64_t remain_chunking_size_ = 0;
        uint64_t cur_offset_ = 0;
        
    public:
        uint64_t _total_chunk_num = 0;
        uint64_t _total_file_size = 0;

        /**
         * @brief Construct a new Abs Chunker object
         * 
         */
        AbsChunker();

        /**
         * @brief Destroy the Abs Chunker object
         * 
         */
        virtual ~AbsChunker();

        /**
         * @brief load the data from the file
         * 
         * @param input_file the input file handler
         * @return uint32_t the read size
         */
        virtual uint32_t LoadDataFromFile(ifstream& input_file) = 0;
        
        /**
         * @brief generate a chunk
         * 
         * @param data the buffer to store the chunk data
         * @return uint32_t the chunk size
         */
        virtual uint32_t GenerateOneChunk(uint8_t* data) = 0;
};

#endif