/**
 * @file delta_comp.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DeltaComp
 * @version 0.1
 * @date 2022-07-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DELTA_COMP_H
#define EDRSTORE_DELTA_COMP_H

#include "../database/db_factory.h"
#include "../configure.h"

#include "../../third/xdelta/xdelta3.h"

class DeltaComp {
    private:
        string my_name_ = "DeltaComp";
        int delta_flag_ = XD3_NOCOMPRESS;

    public:
        /**
         * @brief Construct a new Delta Comp object
         * 
         */
        DeltaComp();

        /**
         * @brief Destroy the Delta Comp object
         * 
         */
        ~DeltaComp();

        /**
         * @brief perform delta encoding
         * 
         * @param base_chunk base chunk data
         * @param base_size base chunk size
         * @param input_chunk input chunk
         * @param input_size input chunk size
         * @param delta_chunk delta chunk data <output>
         * @return uint32_t delta chunk size
         */
        uint32_t DeltaEncode(uint8_t* base_chunk, uint32_t base_size,
            uint8_t* input_chunk, uint32_t input_size, uint8_t* delta_chunk);
        
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
        uint32_t DeltaDecode(uint8_t* base_chunk, uint32_t base_size,
            uint8_t* delta_chunk, uint32_t delta_size, uint8_t* output_chunk);
};

#endif