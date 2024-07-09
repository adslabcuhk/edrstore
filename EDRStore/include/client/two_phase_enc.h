/**
 * @file two_phase_enc.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of two-phase enc
 * @version 0.1
 * @date 2022-05-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_TWO_PHASE_ENC_H
#define EDRSTORE_TWO_PHASE_ENC_H

#include "../crypto/crypto_util.h"

class TwoPhaseEnc {
    private:
        string my_name_ = "TwoPhaseEnc";
        EVP_CIPHER_CTX* cipher_ctx_;

        // for crypto
        CryptoUtil* crypto_util_ctr_;
        CryptoUtil* crypto_util_ecb_;
        uint8_t iv_[CRYPTO_BLOCK_SIZE];
    public:
        /**
         * @brief Construct a new Two Phase Enc object
         * 
         */
        TwoPhaseEnc();

        /**
         * @brief Destroy the Two Phase Enc object
         * 
         */
        ~TwoPhaseEnc();

        /**
         * @brief using two-phase to enc chunk
         * 
         * @param plain_chunk the plain chunk
         * @param size the chunk size
         * @param key the enc key
         * @param enc_chunk the encrypted chunk
         * @return uint32_t the encrypted chunk size
         */
        uint32_t TwoPhaseEncChunk(uint8_t* plain_chunk, uint32_t size, uint8_t* key,
            uint8_t* enc_chunk);

        /**
         * @brief using two-phase to dec chunk
         * 
         * @param enc_chunk the encrypted chunk
         * @param size the chunk size
         * @param key the dec key
         * @param plain_chunk the plain chunk
         * @return uint32_t the plain chunk size
         */
        uint32_t TwoPhaseDecChunk(uint8_t* enc_chunk, uint32_t size, uint8_t* key, 
            uint8_t* plain_chunk);
};

#endif