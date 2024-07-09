/**
 * @file crypto_util.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define interfaces of crypto module (hash&encryption)
 * @version 0.1
 * @date 2019-12-19
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef MY_CODEBASE_CRYPTO_UTIL_H
#define MY_CODEBASE_CRYPTO_UTIL_H

#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include "../data_structure.h"
#include "../define.h"

using namespace std;

static const unsigned char gcm_aad[] = {
    0x4d, 0x23, 0xc3, 0xce, 0xc3, 0x34, 0xb4, 0x9b, 0xdb, 0x37, 0x0c, 0x43,
    0x7f, 0xec, 0x78, 0xde
};

class CryptoUtil {
    private:
        string my_name_ = "Crypto";
        // the type of cipher
        ENCRYPT_SET cipher_type_;
        // the type of hash 
        HASH_SET hash_type_;

        // update the pKey
        EVP_PKEY* p_key_;

    public:
        /**
         * @brief Construct a new Crypto Util object
         * 
         * @param cipher_type the cipher type
         * @param hash_type the hasher type
         */
        CryptoUtil(int cipher_type, int hash_type);

        /**
         * @brief Destroy the Crypto Primitive object
         * 
         */
        ~CryptoUtil();

        /**
         * @brief generate the hash
         * 
         * @param ctx the hasher ctx
         * @param data the input data buffer
         * @param size the input data size
         * @param hash the output hash <return>
         */
        void GenerateHash(EVP_MD_CTX* ctx, uint8_t* data, uint32_t size, uint8_t* hash);

        /**
         * @brief encrypt the data with the key and iv
         * 
         * @param ctx the cipher ctx
         * @param data the input data buffer
         * @param size the input data size
         * @param key the enc key
         * @param iv the input iv
         * @param cipher the output cipher <return>
         * @return uint32_t the cipher size
         */
        uint32_t EncryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* data, uint32_t size, 
            uint8_t* key, uint8_t* iv, uint8_t* cipher);

        /**
         * @brief decrypt the cipher with the key and iv
         * 
         * @param ctx the cipher ctx
         * @param cipher the input cipher buffer
         * @param size the input cipher size
         * @param key the dec key
         * @param iv the input iv
         * @param data the output data <return>
         * @return uint32_t the data size
         */
        uint32_t DecryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* cipher, const int size, 
            uint8_t* key, uint8_t* iv, uint8_t* data);

        /**
         * @brief generate the hmac of the data
         * 
         * @param ctx the hasher ctx
         * @param data the input data buffer 
         * @param size the input data size
         * @param p_key the private key
         * @param hmac the output hmac <return>
         */
        void GenerateHMAC(EVP_MD_CTX* ctx, uint8_t* data, const int size,
            EVP_PKEY* p_key, uint8_t* hmac);
};

#endif