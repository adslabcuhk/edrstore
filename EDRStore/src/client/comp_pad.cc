/**
 * @file comp_pad.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of CompPad
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/comp_pad.h"

/**
 * @brief Construct a new CompPad object
 * 
 */
CompPad::CompPad() {
    compress_util_ = new CompressUtil(COMPRESSION_TYPE, COMPRESSION_LEVEL);
}

/**
 * @brief Destroy the CompPad object
 * 
 */
CompPad::~CompPad() {
    delete compress_util_;
}

/**
 * @brief padding base on the seed
 * 
 * @param padding_seed the seed
 * @param original_size original data size
 * @param compressed_size compressed data size
 * @param compressed_data compressed data
 * @param uint32_t size after padding
 */
uint32_t CompPad::PaddingWithSeed(uint64_t padding_seed, uint32_t original_size,
    uint32_t compressed_size, uint8_t* compressed_data) {
    default_random_engine eng{padding_seed};
    uniform_int_distribution<uint32_t> random_range(0, UINT32_MAX);

    uint32_t padding_limit = min<uint32_t>(
            original_size - compressed_size, max_padding_size_);
    uint32_t padding_blk_num;
    if (padding_limit < sizeof(uint32_t)) {
        padding_blk_num = 0;
    } else {
        padding_blk_num = (padding_seed % padding_limit) / sizeof(uint32_t);
        if (padding_blk_num == 0) {
            padding_blk_num = 1;
        }
    }

    // DEBUG: disable padding here
    // padding_blk_num = 0;

    // init the write pos to the buffer
    uint32_t* write_pos = (uint32_t*)(compressed_data + compressed_size);
    for (size_t i = 0; i < padding_blk_num; i++) {
        *write_pos = random_range(eng);
        write_pos++;
    }

    // write the size wo padding
    memcpy(write_pos, &compressed_size, sizeof(uint32_t));

    return (compressed_size + padding_blk_num * sizeof(uint32_t) +
        sizeof(uint32_t));
}

/**
 * @brief compression with padding
 * 
 * @param input_data input data
 * @param size input size
 * @param output_data ouput data
 * @param padding_seed padding seed
 * @return uint32_t size after padding
 */
uint32_t CompPad::CompressWithPad(uint8_t* input_data, uint32_t size,
    uint8_t* output_data, uint64_t padding_seed) {
    uint32_t size_after_pad = 0;
    uint32_t compressed_size = 0;
    compress_util_->CompressOneChunk(input_data, size, output_data,
        &compressed_size);

    size_after_pad = this->PaddingWithSeed(padding_seed, size,
        compressed_size, output_data);
    return size_after_pad;
}

/**
 * @brief decompression with padding
 * 
 * @param input_data input data
 * @param size input size
 * @param output_data output data
 * @return uint32_t size before padding
 */
uint32_t CompPad::DecompressWithPad(uint8_t* input_data, uint32_t size,
    uint8_t* output_data) {
    uint32_t size_before_pad = 0;
    uint32_t compressed_size = 0;

    // read the compressed size from the last 4 byte
    memcpy(&compressed_size, input_data + size - sizeof(uint32_t),
        sizeof(uint32_t));
    compress_util_->DecompressOneChunk(input_data, compressed_size,
        output_data, &size_before_pad);

    return size_before_pad;
}