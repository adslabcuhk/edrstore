/**
 * @file dual_dedup_thd.cc
 * @author Jia Zhao
 * @brief define the interfaces of DualDedupThd 
 * @version 0.1
 * @date 2022-12-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DUAL_DEDUP_H
#define EDRSTORE_DUAL_DEDUP_H

#include "../configure.h"
#include "../database/db_factory.h"
#include "../reduction/dedup_detect.h"
#include "../chunker/finesse_util.h"
#include "client_var.h"

extern Configure config;

class DualDedupThd{
    private:
        string my_name_ = "DualDedupThd";

        // for deduplication
        AbsDatabase* fp_2_addr_db_;
        DedupDetect* dedup_util_;

        // for feature computation
        FinesseUtil* finesse_util_;

        // for fingerprinting
        CryptoUtil* crypto_util_;

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
        struct timeval _cipher_feature_stime;
        struct timeval _cipher_feature_etime;
        double _total_cipher_feature_time = 0;
        uint64_t _total_cipher_feature_data_size = 0;

        struct timeval _dedup_stime;
        struct timeval _dedup_etime;
        double _total_dedup_time = 0;
#endif

        DualDedupThd(AbsDatabase* fp_2_addr_db);

        ~DualDedupThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client var
         */
        void Run(ClientVar* cur_client);

};

#endif