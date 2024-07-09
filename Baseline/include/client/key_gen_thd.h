/**
 * @file key_gen_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of KeyGenThd
 * @version 0.1
 * @date 2022-06-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_KEY_GEN_THD_H
#define EDRSTORE_KEY_GEN_THD_H

#include "mle.h"
#include "../message_queue/mq_factory.h"
#include "../data_structure.h"
#include "../network/ssl_conn.h"
#include "../configure.h"

extern Configure config;

class KeyGenThd {
    private:
        string my_name_ = "KeyGenThd";

        // config
        uint64_t send_chunk_batch_size_ = 0;
        SendMsgBuffer_t send_buf_;
        SendMsgBuffer_t recv_buf_;
        uint32_t method_type_;

        // to batch the plaintext chunk
        vector<Chunk_t> chunk_buf_;

        // for communication
        SSLConnection* km_channel_;
        pair<int, SSL*> km_conn_record_;
        SSL* km_ssl_;

        // for encryption
        MLE* mle_util_;

        /**
         * @brief add the chunk to the batch plaintext chunk buffer
         * 
         * @param input_chunk the input chunk
         * @param output_MQ the output MQ
         */
        void AddChunkToBuf(Chunk_t& input_chunk,
            AbsMQ<KeyGen2SelectComp_t>* output_MQ);

        /**
         * @brief process a batch of chunks
         * 
         * @param output_MQ the output MQ
         */
        void ProcessBatch(AbsMQ<KeyGen2SelectComp_t>* output_MQ);
    
    public:
        /**
         * @brief Construct a new KeyGenThd object
         * 
         * @param km_channel the key manager connection channel
         * @param km_conn_record the key manager connection channel
         * @param method_type the key generation method 
         */
        KeyGenThd(SSLConnection* km_channel, pair<int, SSL*> km_conn_record,
            uint32_t method_type);

        /**
         * @brief Destroy the KeyGenThd object
         * 
         */
        ~KeyGenThd();

        /**
         * @brief the main thread
         * 
         * @param input_MQ the input MQ
         * @param output_MQ the output MQ
         */
        void Run(AbsMQ<FeatureChunk_t>* input_MQ,
            AbsMQ<EncFeatureChunk_t>* output_MQ);
};

#endif