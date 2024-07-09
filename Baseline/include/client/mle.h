/**
 * @file mle.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of MLE
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_MLE_H
#define EDRSTORE_MLE_H

#include "../define.h"
#include "../crypto/crypto_util.h"

using namespace std;

class MLE {
    private:
        string my_name_ = "MLE";
        uint8_t iv_[CHUNK_HASH_SIZE];
        CryptoUtil* crypto_util_;
        EVP_CIPHER_CTX* cipher_ctx_; 
    
    public:
        /**
         * @brief Construct a new MLE object
         * 
         */
        MLE();

        /**
         * @brief Destroy the MLE object
         * 
         */
        ~MLE();

        /**
         * @brief using MLE to enc chunk
         * 
         * @param plain_chunk the plain chunk
         * @param size the chunk size
         * @param key the enc key
         * @param enc_chunk the encrypted chunk
         */
        void EncChunk(uint8_t* plain_chunk, uint32_t size, uint8_t* key,
            uint8_t* enc_chunk);

        /**
         * @brief using MLE to dec chunk
         * 
         * @param enc_chunk the encrypted chunk
         * @param size the chunk size 
         * @param key the dec key
         * @param plain_chunk the plain chunk
         */
        void DecChunk(uint8_t* enc_chunk, uint32_t size, uint8_t* key,
            uint8_t* plain_chunk);
};

#endif