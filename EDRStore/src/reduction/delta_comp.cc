/**
 * @file delta_comp.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DeltaComp
 * @version 0.1
 * @date 2022-07-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/reduction/delta_comp.h"

/**
 * @brief Construct a new Delta Comp object
 * 
 * @param storage_core storage core
 */
DeltaComp::DeltaComp() {
}

/**
 * @brief Destroy the Delta Comp object
 * 
 */
DeltaComp::~DeltaComp() {
}

/**
 * @brief perform delta encoding
 * 
 * @param base_chunk base chunk data
 * @param base_size base chunk size
 * @param input_chunk input chunk
 * @param input_size input chunk size
 * @param delta_chunk output delta chunk
 * @return uint32_t delta chunk size
 */
uint32_t DeltaComp::DeltaEncode(uint8_t* base_chunk, uint32_t base_size,
    uint8_t* input_chunk, uint32_t input_size, uint8_t* delta_chunk) {
    uint64_t ret_size = 0;
    int ret = xd3_encode_memory(input_chunk, input_size, base_chunk, base_size,
        delta_chunk, &ret_size, ENC_MAX_CHUNK_SIZE, delta_flag_);
    if (ret != 0) {
        tool::Logging(my_name_.c_str(), "delta encoding fails: %d.\n",
            ret);
        exit(EXIT_FAILURE);
    }
    return ret_size;
}

/**
 * @brief perform delta decoding
 * 
 * @param base_chunk base chunk data
 * @param base_size base chunk size
 * @param delta_chunk delta chunk data
 * @param delta_size delta chunk size
 * @param output_chunk output chunk data <output>
 * @return uint32_t output chunk size
 */
uint32_t DeltaComp::DeltaDecode(uint8_t* base_chunk, uint32_t base_size,
    uint8_t* delta_chunk, uint32_t delta_size, uint8_t* output_chunk) {
    uint64_t ret_size = 0;
    int ret = xd3_decode_memory(delta_chunk, delta_size, base_chunk, base_size,
        output_chunk, &ret_size, ENC_MAX_CHUNK_SIZE, delta_flag_);
    if (ret != 0) {
        tool::Logging(my_name_.c_str(), "delta decoding fails: %d.\n",
            ret);
        exit(EXIT_FAILURE);
    }
    return ret_size;
}