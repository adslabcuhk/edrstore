/**
 * @file basic_km.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of the basic key manager
 * @version 0.1
 * @date 2022-04-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef BASIC_KM_H
#define BASIC_KM_H

#include "define.h"
#include "configure.h"
#include "../data_structure.h"
#include "../network/ssl_conn.h"
#include "../crypto/crypto_util.h"
#include "../database/db_factory.h"
#include "../reduction/similar_policy.h"

extern Configure config;

class BasicKM {
    private:
        string my_name_ = "BasicKM";

        SSLConnection* km_channel_;

        // config
        uint64_t send_chunk_batch_size_ = 0;

        // for crypto
        CryptoUtil* crypto_util_;
        uint8_t global_secret_[CHUNK_HASH_SIZE];
        uint8_t global_padding_secret_[CHUNK_HASH_SIZE];

        AbsDatabase* feature_2_key_index_;

        SimilarPolicy* similar_policy_;

        /**
         * @brief convert the fp to value
         * 
         * @param fp chunk fp
         * @param size chunk size
         * @return uint64_t fp value
         */
        uint64_t ConvertFp2Val(uint8_t* fp, uint32_t size);
 
    public:
        // for statistics
        uint64_t _total_key_gen_num = 0;
        uint64_t _total_similar_chunk_num = 0;

        /**
         * @brief Construct a new Basic KM object
         * 
         * @param km_channel the key generation channel
         * @param feature_2_key_index the feature index
         */
        BasicKM(SSLConnection* km_channel, AbsDatabase* feature_2_key_index);

        /**
         * @brief Destroy the Basic KM object
         * 
         */
        ~BasicKM();

        /**
         * @brief the main process of the client
         * 
         * @param key_client_ssl the client ssl
         */
        void Run(SSL* key_client_ssl);
};

#endif