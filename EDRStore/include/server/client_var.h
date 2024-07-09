/**
 * @file client_var.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of curClient
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_CLIENT_VAR_H
#define EDRSTORE_CLIENT_VAR_H

#include "inform_cache.h"
#include "../configure.h"
#include "../message_queue/mq_factory.h"
#include "../network/ssl_conn.h"
#include "../crypto/crypto_util.h"
#include "../readCache.h"
#include "../chunker/rabin_poly.h"

extern Configure config;

class ClientVar {
    private:
        string my_name_ = "ClientVar";
        int opt_type_; // the operation type (upload / download)
        uint64_t send_chunk_batch_size_;
        uint64_t send_recipe_batch_size_;
        string recipe_path_;

        RabinFPUtil* rabin_util_;

        uint32_t MQ_TYPE_ = LCK_FREE_MQ;
        MQFactory<WrappedChunk_t> wrapped_chunk_mq_factory_;
        MQFactory<Reader2Decoder_t> reader_2_decoder_mq_factory_;

        /**
         * @brief init the var related to the upload
         * 
         */
        void InitUploadBuffer();

        /**
         * @brief init var related to the download
         * 
         */
        void InitDownloadBuffer();

        /**
         * @brief destroy the var related to the upload
         * 
         */
        void DestroyUploadBuffer();

        /**
         * @brief destroy the var related to the download
         * 
         */
        void DestroyDownloadBuffer();

    public:
        uint32_t _client_id;
        // for crypto
        EVP_MD_CTX* _md_ctx;
        EVP_CIPHER_CTX* _cipher_ctx;

        // for handling file recipe
        ofstream _recipe_write_hdl;
        ifstream _recipe_read_hdl;

        // common container cache
        ReadCache* _container_cache;
        InformCache* _inform_cache;

        // upload var
        Container_t _cur_container;
        RabinCtx_t _rabin_ctx;
        SendMsgBuffer_t _recv_chunk_buf;
        BatchBuf_t _recipe_batch;
        // AbsMQ<WrappedChunk_t>* _recv_2_comp_mq;
        // AbsMQ<WrappedChunk_t>* _comp_2_writer_mq;
        AbsMQ<WrappedChunk_t>* _recv_2_dual_mq;
        AbsMQ<WrappedChunk_t>* _dual_2_comp_mq;
        AbsMQ<WrappedChunk_t>* _comp_2_writer_mq;

        // download var
        SendMsgBuffer_t _send_chunk_buf;
        uint8_t* _read_recipe_buf;
        AbsMQ<Reader2Decoder_t>*_reader_2_decoder_mq;

        SSL* _client_ssl; // SSL connection

        uint64_t* _total_cache_size;

        /**
         * @brief Construct a new Client Var object
         * 
         * @param client_id the client ID
         * @param client_ssl the client SSL
         * @param opt_type the operation type (upload / download)
         * @param recipe_path the file recipe path
         */
        ClientVar(uint32_t client_id, SSL* client_ssl,
            int opt_type, string& recipe_path);

        /**
         * @brief Destroy the Client Var object
         * 
         */
        ~ClientVar();
};

#endif