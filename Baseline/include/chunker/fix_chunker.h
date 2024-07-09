/**
 * @file fix_chunker.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of FixChunker
 * @version 0.1
 * @date 2022-06-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_FIX_CHUNKER_H
#define MY_CODEBASE_FIX_CHUNKER_H

#include "abs_chunker.h"

class FixChunker : public AbsChunker {
    protected: 
        string my_name_ = "FixChunker";
    
    public:
        /**
         * @brief Construct a new Fix Chunker object
         * 
         */
        FixChunker();

        /**
         * @brief Destroy the Fix Chunker object
         * 
         */
        ~FixChunker();

        /**
         * @brief load the data from the file
         * 
         * @param input_file the input file handler
         * @return uint32_t the read size
         */
        uint32_t LoadDataFromFile(ifstream& input_file);

        /**
         * @brief generate a chunk
         * 
         * @param data the buffer to store the chunk data
         * @return uint32_t the chunk size
         */
        uint32_t GenerateOneChunk(uint8_t* data);
};

#endif