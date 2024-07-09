/**
 * @file plain_similar_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief  define the thread to compute the plaintext chunk features 
 * @version 0.1
 * @date 2022-06-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef PLAIN_SIMILAR_THD_H
#define PLAIN_SIMILAR_THD_H

#include "../chunker/finesse_util.h"
#include "../chunker/rabin_poly.h"
#include "../message_queue/mq_factory.h"
#include "../data_structure.h"
#include "../configure.h"

extern Configure config;

class PlainSimilarThd {
    private:
        string my_name_ = "PlainSimilarThd";

        FinesseUtil* finesse_util_;
        RabinCtx_t rabin_ctx_;  
        RabinFPUtil* rabin_util_;

    public:
        /**
         * @brief Construct a new PlainSimilarThd object
         * 
         */
        PlainSimilarThd();

        /**
         * @brief Destroy the PlainSimilarThd object
         * 
         */
        ~PlainSimilarThd();

        /**
         * @brief the main thread
         * 
         * @param input_MQ the input MQ
         * @param output_MQ the output MQ
         */
        void Run(AbsMQ<Chunk_t>* input_MQ, AbsMQ<FeatureChunk_t>* output_MQ);
};

#endif