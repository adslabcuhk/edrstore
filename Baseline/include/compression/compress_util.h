/**
 * @file compression.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface for local compression
 * @version 0.1
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_COMPRESS_H
#define MY_CODEBASE_COMPRESS_H

#include "../define.h"
#include "../const_var.h"

// for compression header 
#include <lz4.h>
#include <zstd.h>
#include <zlib.h>

using namespace std;

class CompressUtil {
    private:
        string my_name_ = "CompressUtil";
        uint32_t compress_type_ = 0;
        uint32_t compress_level_ = 0;

    public:

        /**
         * @brief Construct a new Compress Util object
         * 
         * @param compress_type the compression type
         * @param compress_level the compression level
         */
        CompressUtil(uint32_t compress_type, uint32_t compress_level);

        /**
         * @brief compress a chunk
         * 
         * @param chunk_data the chunk data
         * @param chunk_size the chunk size
         * @param output_data the chunk data
         * @param output_size the chunk size
         * @return true it can compress
         * @return false it cannot compress
         */
        bool CompressOneChunk(uint8_t* chunk_data, uint32_t chunk_size, 
            uint8_t* output_data, uint32_t* output_size);

        /**
         * @brief uncompress a chunk
         * 
         * @param compressed_data the compressed data
         * @param compressed_size the compressed size
         * @param uncompressed_data the uncompressed data
         * @param uncompressed_size the uncompressed size
         */
        void DecompressOneChunk(uint8_t* compressed_data, uint32_t compressed_size, 
            uint8_t* uncompressed_data, uint32_t* uncompressed_size);

        /**
         * @brief Destroy the Compress Util object
         * 
         */
        ~CompressUtil();
};

#endif