/**
 * @file finesse.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of finesse
 * @version 0.1
 * @date 2022-06-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_FINESSE_H
#define MY_CODEBASE_FINESSE_H

#include "../define.h"
#include "../configure.h"
#include "rabin_poly.h"
#include "xxhash64.h"

using namespace std;

extern Configure config;

class FinesseUtil {
    private:
        string my_name_ = "FinesseUtil";

        // configure
        uint64_t super_feature_per_chunk_;
        uint64_t feature_per_chunk_;
        uint64_t feature_per_super_feature_;

        RabinFPUtil* rabin_util_;

    public:
        /**
         * @brief Construct a new FinesseUtil object
         * 
         * @param super_feature_per_chunk super feature per chunk 
         * @param feature_per_chunk feature per chunk 
         * @param feature_per_super_feature feature per super feature
         */
        FinesseUtil(uint64_t super_feature_per_chunk, uint64_t feature_per_chunk,
            uint64_t feature_per_super_feature);

        /**
         * @brief Destroy the FinesseUtil object
         * 
         */
        ~FinesseUtil();

        /**
         * @brief compute the features from the chunk
         *
         * @param ctx the rabin ctx
         * @param data the chunk data
         * @param size the chunk size
         * @param features the chunk features
         */
        void ExtractFeature(RabinCtx_t& ctx, uint8_t* data, uint32_t size,
            uint64_t* features);
};

#endif