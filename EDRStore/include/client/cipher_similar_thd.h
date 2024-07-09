/**
 * @file cipher_similar_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the thread to compute the ciphertext chunk features
 * @version 0.1
 * @date 2022-06-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef CIPHER_SIMILAR_THD_H
#define CIPHER_SIMILAR_THD_H

#include "../chunker/finesse_util.h"
#include "../chunker/rabin_poly.h"
#include "../message_queue/mq_factory.h"
#include "../data_structure.h"

extern Configure config;

class CipherSimilarThd {
    private:
        string my_name_ = "CipherSimilarThd";

        FinesseUtil* finesse_util_;
        RabinCtx_t rabin_ctx_;
        RabinFPUtil* rabin_util_;
    
    public:
#ifdef EDR_BREAKDOWN
        struct timeval _cipher_feature_stime;
        struct timeval _cipher_feature_etime;
        double _total_cipher_feature_time = 0;
        uint64_t _total_cipher_feature_size = 0;
#endif
        /**
         * @brief Construct a new Cipher Similar Thd object
         * 
         */
        CipherSimilarThd();

        /**
         * @brief Destroy the Cipher Similar Thd object
         * 
         */
        ~CipherSimilarThd();

        /**
         * @brief the main thread
         * 
         * @param input_MQ the input MQ
         */
        void Run(AbsMQ<EncFeatureChunk_t>* input_MQ, AbsMQ<EncFeatureChunk_t>* output_MQ);
};

#endif