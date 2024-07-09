/**
 * @file data_retriever_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DataRetrieverThd
 * @version 0.1
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DATA_RETRIEVER_THD_H
#define EDRSTORE_DATA_RETRIEVER_THD_H

#include "mle.h"
#include "comp_pad.h"
#include "../network/ssl_conn.h"
#include "../message_queue/mq_factory.h"
#include "../configure.h"

extern Configure config;

class DataRetrieverThd {
    private:
        string my_name_ = "DataRetrieverThd";

        uint32_t method_type_;

        // for config
        uint64_t send_chunk_batch_size_;
        uint64_t send_recipe_batch_size_;
        uint32_t client_id_;

        // the recv chunk buf
        SendMsgBuffer_t recv_chunk_buf_;

        // for communication
        SSLConnection* server_channel_;
        pair<int, SSL*> server_conn_record_;
        SSL* server_ssl_;

        // for key recipe
        ifstream key_recipe_hdl_;
        BatchBuf_t key_recipe_buf_;
        uint32_t remain_recipe_num_ = 0;

        // for recipe head
        FileRecipeHead_t recipe_head_;

        // for decryption
        MLE* mle_util_;

        // for compression padding
        CompPad* comp_pad_;

        /**
         * @brief process a batch of chunks
         * 
         * @param output_MQ output MQ
         */
        void ProcessBatchChunks(AbsMQ<Retriever2Writer_t>* output_MQ);

        /**
         * @brief process a chunk
         * 
         * @param input_chunk input chunk
         * @param size chunk size
         * @param output_chunk output chunk
         */
        void ProcessChunk(uint8_t* input_chunk, uint32_t size, SendChunk_t* output_chunk);

        /**
         * @brief fetch key recipe
         * 
         */
        void FetchKeyRecipe();

    public:
        uint64_t _total_recv_chunk_num = 0;
        uint64_t _total_recv_data_size = 0;

        /**
         * @brief Construct a new DataRetrieverThd object
         * 
         * @param server_channel server channel
         * @param server_conn_record connection record
         * @param file_name_hash file name hash
         * @param method_type method type
         */
        DataRetrieverThd(SSLConnection* server_channel, pair<int, SSL*>
            server_conn_record, uint8_t* file_name_hash,
            uint32_t method_type);

        /**
         * @brief Destroy the DataRetrieverThd object
         * 
         */
        ~DataRetrieverThd();

        /**
         * @brief the main process
         * 
         * @param output_MQ output MQ
         */
        void Run(AbsMQ<Retriever2Writer_t>* output_MQ);

        /**
         * @brief download login with the file name hash
         * 
         * @param file_name_hash the file name hash
         */
        void DownloadLogin(uint8_t* file_name_hash);
};

#endif