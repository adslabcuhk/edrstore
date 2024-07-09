/**
 * @file comp_pad.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of CompPad
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_COMP_PAD_H
#define EDRSTORE_COMP_PAD_H

#include "../define.h"
#include "../data_structure.h"
#include "../compression/compress_util.h"

class CompPad {
    private:
        string my_name_ = "CompPad";
        uint64_t max_padding_size_ = 255;

        CompressUtil* compress_util_;

        /**
         * @brief padding base on the seed
         * 
         * @param padding_seed the seed
         * @param original_size original data size
         * @param compressed_size compressed data size
         * @param compressed_data compressed data
         */
        uint32_t PaddingWithSeed(uint64_t padding_seed, uint32_t original_size,
            uint32_t compressed_size, uint8_t* compressed_data);

    public:
        /**
         * @brief Construct a new CompPad object
         * 
         */
        CompPad();

        /**
         * @brief Destroy the CompPad object
         * 
         */
        ~CompPad();

        /**
         * @brief compression with padding
         * 
         * @param input_data input data
         * @param size input size
         * @param output_data ouput data
         * @param padding_seed padding seed
         * @return uint32_t size after padding
         */
        uint32_t CompressWithPad(uint8_t* input_data, uint32_t size,
            uint8_t* output_data, uint64_t padding_seed);

        /**
         * @brief decompression with padding
         * 
         * @param input_data input data
         * @param size input size
         * @param output_data output data
         * @return uint32_t size before padding
         */
        uint32_t DecompressWithPad(uint8_t* input_data, uint32_t size,
            uint8_t* output_data);
};

#endif