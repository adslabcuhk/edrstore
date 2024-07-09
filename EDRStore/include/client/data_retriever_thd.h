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

#include "two_phase_enc.h"
#include "comp_pad.h"
#include "../network/ssl_conn.h"
#include "../message_queue/mq_factory.h"
#include "../configure.h"
#include "../compression/compress_util.h"
#include "../reduction/delta_comp.h"

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
        TwoPhaseEnc* two_phase_enc_;

        // for decompression
        CompPad* comp_pad_;

        // for delta compression
        DeltaComp* delta_comp_;

        /**
         * @brief process a batch of chunks (Full EDR)
         * 
         * @param output_MQ output MQ
         */
        void FullEDR(AbsMQ<Retriever2Writer_t>* output_MQ);

        /**
         * @brief process a batch of chunks (only enc)
         * 
         * @param output_MQ output MQ
         */
        void OnlyEncMode(AbsMQ<Retriever2Writer_t>* output_MQ);

        /**
         * @brief process a batch of chunks (comp + enc)
         * 
         * @param output_MQ output MQ
         */
        void EncCompMode(AbsMQ<Retriever2Writer_t>* output_MQ);

        /**
         * @brief process a comp chunk
         * 
         * @param input_chunk input chunk
         * @param size chunk size
         * @param output_chunk output chunk
         * @param key_recipe key recipe
         */
        void ProcCompChunk(uint8_t* input_chunk, uint32_t size,
            SendChunk_t* output_chunk, KeyRecipe_t* key_recipe);

        /**
         * @brief proc an un-comp chunk
         * 
         * @param input_chunk input chunk
         * @param size chunk size
         * @param output_chunk output chunk
         * @param key_recipe key recipe
         */
        void ProcUncompChunk(uint8_t* input_chunk, uint32_t size,
            SendChunk_t* output_chunk, KeyRecipe_t* key_recipe);

        /**
         * @brief process restore base chunk
         * 
         * @param input_chunk input chunk
         * @param size chunk size
         * @param restore_chunk output chunk
         * @param key_recipe key recipe
         */
        void ProcRestoreBaseChunk(uint8_t* input_chunk, uint32_t size,
            SendChunk_t* restore_chunk, KeyRecipe_t* key_recipe);

        /**
         * @brief fetch key recipe
         * 
         */
        void FetchKeyRecipe();

        /**
         * @brief process the end flag 
         * 
         */
        void ProcessEndFlag();

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