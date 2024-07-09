/**
 * @file data_receiver.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DataRecvThd
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DATA_RECEIVER_H
#define EDRSTORE_DATA_RECEIVER_H

#include "../configure.h"
#include "../database/db_factory.h"
#include "../network/ssl_conn.h"
#include "../reduction/dedup_detect.h"
#include "../chunker/finesse_util.h"
#include "client_var.h"

extern Configure config;

class DataRecvThd{
    private:
        string my_name_ = "DataRecvThd";

        // for configure
        uint64_t send_chunk_batch_size_;
        uint64_t send_recipe_batch_size_;
        uint64_t feature_batch_size_ = MAX_CHUNK_SIZE / sizeof(uint64_t);

        // for storage server connection
        SSLConnection* server_channel_;

        // for deduplication
        AbsDatabase* fp_2_addr_db_;
        DedupDetect* dedup_util_;

        // for feature computation
        FinesseUtil* finesse_util_;

        // for fingerprinting
        CryptoUtil* crypto_util_;

        /**
         * @brief process a batch of chunks
         * 
         * @param cur_client current client var
         */
        void ProcessChunks(ClientVar* cur_client);

        /**
         * @brief process the recipe end
         * 
         * @param cur_client current client var
         */
        void ProcessRecipeEnd(ClientVar* cur_client);

        /**
         * @brief add fp to the recipe
         * 
         * @param cur_client current client
         * @param fp chunk fp
         */
        void ProcessRecipe(ClientVar* cur_client, uint8_t* fp);

        /**
         * @brief process evict features
         * 
         * @param cur_client current client
         */
        void ProcessEvictFeature(ClientVar* cur_client);

    public:
        uint64_t _chunk_batch_num = 0;
        uint64_t _total_recv_chunk_num = 0;
        uint64_t _total_recv_data_size = 0;

        // for deduplication
        uint64_t _total_logical_data_size = 0;
        uint64_t _total_logical_chunk_num = 0;
        uint64_t _total_unique_data_size = 0;
        uint64_t _total_unique_chunk_num = 0;
    
#ifdef EDR_BREAKDOWN
        struct timeval _cipher_fp_stime;
        struct timeval _cipher_fp_etime;
        double _total_cipher_fp_time = 0;
        uint64_t _total_cipher_fp_data_size = 0;

        struct timeval _dual_fp_stime;
        struct timeval _dual_fp_etime;
        double _total_dual_fp_time = 0;
        uint64_t _total_dual_fp_data_size = 0;

        struct timeval _cipher_feature_stime;
        struct timeval _cipher_feature_etime;
        double _total_cipher_feature_time = 0;
        uint64_t _total_cipher_feature_data_size = 0;

        struct timeval _dedup_stime;
        struct timeval _dedup_etime;
        double _total_dedup_time = 0;
#endif

        /**
         * @brief Construct a new DataRecvThd object
         * 
         * @param server_channel the storage server channel
         * @param fp_2_addr_db fp to chunk addr index
         */
        DataRecvThd(SSLConnection* server_channel, AbsDatabase* fp_2_addr_db);

        /**
         * @brief Destroy the DataRecvThd object
         * 
         */
        ~DataRecvThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client var
         */
        void Run(ClientVar* cur_client);
};

#endif