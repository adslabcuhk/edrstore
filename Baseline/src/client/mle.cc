/**
 * @file mle.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the implementation of the MLE
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/mle.h"

/**
 * @brief Construct a new MLE object
 * 
 */
MLE::MLE() {
    memset(iv_, 0, CHUNK_HASH_SIZE);
    crypto_util_ = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
    cipher_ctx_ = EVP_CIPHER_CTX_new();
}

/**
 * @brief Destroy the MLE object
 * 
 */
MLE::~MLE() {
    delete crypto_util_;
    EVP_CIPHER_CTX_free(cipher_ctx_);
}

/**
 * @brief using MLE to enc chunk
 * 
 * @param plain_chunk the plain chunk
 * @param size the chunk size
 * @param key the enc key
 * @param enc_chunk the encrypted chunk
 */
void MLE::EncChunk(uint8_t* plain_chunk, uint32_t size, uint8_t* key,
    uint8_t* enc_chunk) {
    crypto_util_->EncryptWithKeyIV(cipher_ctx_, plain_chunk, size,
        key, iv_, enc_chunk);
    return ;
}

/**
 * @brief using MLE to dec chunk
 * 
 * @param enc_chunk the encrypted chunk
 * @param size the chunk size 
 * @param key the dec key
 * @param plain_chunk the plain chunk
 */
void MLE::DecChunk(uint8_t* enc_chunk, uint32_t size, uint8_t* key,
    uint8_t* plain_chunk) {
    crypto_util_->DecryptWithKeyIV(cipher_ctx_, enc_chunk, size,
        key, iv_, plain_chunk);
    return ;
}