/**
 * @file crypto_util.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of CryptoUtil
 * @version 0.1
 * @date 2022-05-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/crypto/crypto_util.h"

/**
 * @brief Construct a new Crypto Util object
 * 
 * @param cipher_type the cipher type
 * @param hash_type the hasher type
 */
CryptoUtil::CryptoUtil(int cipher_type, int hash_type) {
    // for openssl optimization
    OPENSSL_init_crypto(0, NULL);
    ERR_load_crypto_strings();

    cipher_type_ = static_cast<ENCRYPT_SET>(cipher_type);
    hash_type_ = static_cast<HASH_SET>(hash_type);

    string deriveStr = "password";
    p_key_ = EVP_PKEY_new_raw_private_key(EVP_PKEY_HMAC, NULL, (uint8_t*)deriveStr.c_str(),
        deriveStr.size());
}

/**
 * @brief Destroy the Crypto Primitive object
 * 
 */
CryptoUtil::~CryptoUtil() {
    ERR_free_strings();
    EVP_PKEY_free(p_key_);
}

/**
 * @brief generate the hash
 * 
 * @param ctx the hasher ctx
 * @param data the input data buffer
 * @param size the input data size
 * @param hash the output hash <return>
 */
void CryptoUtil::GenerateHash(EVP_MD_CTX* ctx, uint8_t* data, uint32_t size, uint8_t* hash) {
    int expectedHashSize = 0;
    switch (hash_type_) {
        case SHA_1: {
            if (!EVP_DigestInit_ex(ctx, EVP_sha1(), NULL)) {
                tool::Logging(my_name_.c_str(), "hash init error.\n");
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 20;
            break;
        }
        case SHA_256: {
            if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL)) {
                tool::Logging(my_name_.c_str(), "hash init error.\n");
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 32;
            break;
        }
        case MD5: {
            if (!EVP_DigestInit_ex(ctx, EVP_md5(), NULL)) {
                tool::Logging(my_name_.c_str(), "hash init error.\n");
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 16;
            break;
        }
    }

    if (!EVP_DigestUpdate(ctx, data, size)) {
        tool::Logging(my_name_.c_str(), "hash error.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t hashSize;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashSize)) {
        tool::Logging(my_name_.c_str(), "hash error.\n");
        exit(EXIT_FAILURE);
    }

    if (hashSize != expectedHashSize) {
        tool::Logging(my_name_.c_str(), "hash size error.\n");
        exit(EXIT_FAILURE);
    }
    
    EVP_MD_CTX_reset(ctx);
    return ;
}

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
uint32_t CryptoUtil::EncryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* data, uint32_t size, 
    uint8_t* key, uint8_t* iv, uint8_t* cipher) {
    int cipher_len = 0;
    int len = 0;

    switch (cipher_type_) {
        case AES_128_CFB: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_CFB: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_CBC: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_128_CBC: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_256_GCM: {
            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_EncryptUpdate(ctx, NULL, &cipher_len, gcm_aad, sizeof(gcm_aad));
            break;
        }
        case AES_128_GCM: {
            EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_EncryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_EncryptUpdate(ctx, NULL, &cipher_len, gcm_aad, sizeof(gcm_aad));
            break;
        }
        case AES_256_CTR: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            } 
            break;
        }
        case AES_128_CTR: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_ECB: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), NULL,
                key, NULL)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_128_ECB: {
            if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL,
                key, NULL)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
    }

    // encrypt the plaintext
    if (!EVP_EncryptUpdate(ctx, cipher, &cipher_len, data, 
        size)) {
        ERR_print_errors_fp(stderr);
        tool::Logging(my_name_.c_str(), "encryption error.\n");
        exit(EXIT_FAILURE);
    }
    EVP_EncryptFinal_ex(ctx, cipher + cipher_len, &len);
    cipher_len += len;
	
    EVP_CIPHER_CTX_reset(ctx);
    return static_cast<uint32_t>(cipher_len);
}

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
uint32_t CryptoUtil::DecryptWithKeyIV(EVP_CIPHER_CTX* ctx, uint8_t* cipher, const int size, 
    uint8_t* key, uint8_t* iv, uint8_t* data) {
    int plain_len;
    int len;

    switch (cipher_type_) {
        case AES_128_CFB: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, 
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_CFB: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_CBC: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_128_CBC: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_128_GCM: {
            EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_DecryptUpdate(ctx, NULL, &plain_len, gcm_aad, sizeof(gcm_aad));
            break;
        }
        case AES_256_GCM: {
            EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, CRYPTO_BLOCK_SIZE, NULL);
            if (!EVP_DecryptInit_ex(ctx, NULL, NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_DecryptUpdate(ctx, NULL, &plain_len, gcm_aad, sizeof(gcm_aad));
            break;
        }
        case AES_256_CTR: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_128_CTR: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL,
                key, iv)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case AES_256_ECB: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_ecb(), NULL,
                key, NULL)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
        case AES_128_ECB: {
            if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL,
                key, NULL)) {
                tool::Logging(my_name_.c_str(), "init error.\n");
                exit(EXIT_FAILURE);
            }
            EVP_CIPHER_CTX_set_padding(ctx, 1);
            break;
        }
    }

    // decrypt the plaintext
    if (!EVP_DecryptUpdate(ctx, data, &plain_len, cipher,
        size)) {
        ERR_print_errors_fp(stderr);
        tool::Logging(my_name_.c_str(), "decrypt error.\n");
        exit(EXIT_FAILURE);
    }

    EVP_DecryptFinal_ex(ctx, data + plain_len, &len);
    
    plain_len += len;

    EVP_CIPHER_CTX_reset(ctx);
    return static_cast<uint32_t>(plain_len);
}

/**
 * @brief generate the hmac of the data
 * 
 * @param ctx the hasher ctx
 * @param data the input data buffer 
 * @param size the input data size
 * @param p_key the private key
 * @param hmac the output hmac <return>
 */
void CryptoUtil::GenerateHMAC(EVP_MD_CTX* ctx, uint8_t* data, const int size,
    EVP_PKEY* p_key, uint8_t* hmac) {
    int expectedHashSize = 0;

    switch (hash_type_) {
        case SHA_1: {
            if (EVP_DigestSignInit(ctx, NULL, EVP_sha1(), NULL, p_key) != 1) {
                tool::Logging(my_name_.c_str(), "HMAC init error, error 0x%lx\n", ERR_get_error());
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 20;
            break;
        }
        case SHA_256: {
            if (EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, p_key) != 1) {
                tool::Logging(my_name_.c_str(), "HMAC init error, error 0x%lx\n", ERR_get_error());
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 32;
            break;
        }
        case MD5: {
            if (EVP_DigestSignInit(ctx, NULL, EVP_md5(), NULL, p_key) != 1) {
                tool::Logging(my_name_.c_str(), "HMAC init error, error 0x%lx\n", ERR_get_error());
                exit(EXIT_FAILURE);
            }
            expectedHashSize = 16;
            break;
        }
    }

    if (EVP_DigestSignUpdate(ctx, data, size) != 1) {
        tool::Logging(my_name_.c_str(), "HMAC update error, error 0x%lx\n", ERR_get_error());
        exit(EXIT_FAILURE);
    }

    // first call the buffer should be NULL, and receive the size of the signature
    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx, hmac, &sigLen) != 1) {
        tool::Logging(my_name_.c_str(), "HMAC final error, error 0x%lx\n", ERR_get_error());
        exit(EXIT_FAILURE);
    }

    if (sigLen != expectedHashSize) {
        tool::Logging(my_name_.c_str(), "HMAC sig len cannot match the expected size.\n");
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_reset(ctx);

    return ;
}