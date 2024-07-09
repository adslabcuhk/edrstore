/**
 * @file sender_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the thread to realize the compression and send 
 * @version 0.1
 * @date 2022-06-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_SENDER_THD_H
#define EDRSTORE_SENDER_THD_H

#include "../configure.h"
#include "../network/ssl_conn.h"
#include "../data_structure.h"
#include "../message_queue/mq_factory.h"

extern Configure config;

class SenderThd {
    private:
        string my_name_ = "SenderThd";

        // config
        uint64_t send_chunk_batch_size_ = 0;
        uint64_t send_recipe_batch_size_ = 0;
        uint32_t client_id_;

        // the send chunk buf
        SendMsgBuffer_t send_chunk_buf_;

        // for communication
        SSLConnection* server_channel_;
        pair<int, SSL*> server_conn_record_;
        SSL* server_ssl_;

        // the master key
        uint8_t master_key_[CHUNK_HASH_SIZE] = {0};

        // for key recipe
        ofstream key_recipe_hdl_;
        BatchBuf_t key_recipe_buf_;

        /**
         * @brief send a batch of chunks
         * 
         */
        void SendChunks();

        /**
         * @brief process the end of the recipe
         * 
         * @param input_chunk the input recipe data
         */
        void ProcessRecipeEnd(SendChunk_t* input_chunk);

        /**
         * @brief store the key recipe
         * 
         * @param key_recipe the ptr to the key recipe
         */
        void StoreKeyRecipe(KeyRecipe_t* key_recipe);
    
    public:
        /**
         * @brief Construct a new SenderThd object
         * 
         * @param server_channel the connection to the storage server
         * @param server_conn_record the storage server connection record
         * @param file_name_hash the file name hash
         */
        SenderThd(SSLConnection* server_channel, pair<int, SSL*> server_conn_record,
            uint8_t* file_name_hash);

        /**
         * @brief Destroy the SenderThd object
         * 
         */
        ~SenderThd();

        /**
         * @brief Set the MasterKey object
         * 
         * @param master_key the ptr to the master key
         */
        void SetMasterKey(uint8_t* master_key) {
            memcpy(master_key_, master_key, CHUNK_HASH_SIZE);
            return ;
        }

        /**
         * @brief the main thread
         * 
         * @param the input MQ
         */
        void Run(AbsMQ<SelectComp2Sender_t>* input_MQ);

        /**
         * @brief upload login with the file name hash
         * 
         * @param file_name_hash the file name hash
         */
        void UploadLogin(uint8_t* file_name_hash);
};

#endif