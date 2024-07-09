/**
 * @file server_opt_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of ServerOptThd
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_SERVER_OPT_THD_H
#define EDRSTORE_SERVER_OPT_THD_H

#include <boost/thread/thread.hpp>

// for upload
#include "../server/data_recv_thd.h"
#include "../server/cache_comp_thd.h"
#include "../server/data_writer_thd.h"
#include "../server/dual_dedup_thd.h"

// for download
#include "../server/data_reader_thd.h"
#include "../server/data_decoder_thd.h"

// for basic building blocks
#include "../database/db_factory.h"
#include "../server/client_var.h"
#include "../server/storage_core.h" 
#include "../network/ssl_conn.h"
#include "../configure.h"
#include "../reduction/dedup_detect.h"
#include "../reduction/delta_comp.h"

using namespace std;

extern Configure config;

class ServerOptThd {
    private:
        string my_name_ = "ServerOptThd";
        string server_log_name_ = "server-log";
        string persist_stat_name_ = "persist-stat";

        // for storage server connection channel
        SSLConnection* server_channel_;

        // for storage operation
        StorageCore* storage_core_;

        // for upload thread
        DataRecvThd* data_recv_thd_;
        CacheCompThd* cache_comp_thd_; 
        DataWriterThd* data_writer_thd_; 
        DualDedupThd* dual_dedup_thd_;

        // for download thread
        DataDecoderThd* data_decode_thd_;
        DataReaderThd* data_reader_thd_;

        // for index
        AbsDatabase* fp_2_addr_db_;
        AbsDatabase* feature_2_fp_db_;

        // locks for multiple clients
        unordered_map<int, boost::mutex*> client_lck_idx_;
        std::mutex client_idx_lck_;

        /**
         * @brief lock the mutex of a given client id
         * 
         * @param client_id the client id
         */
        void LockClientID(uint32_t client_id);

        /**
         * @brief unlock the mutex of a given client id
         * 
         * @param client_id the client id
         */
        void UnlockClientID(uint32_t client_id);

        /**
         * @brief check the file status
         * 
         * @param recipe_path the full recipe path
         * @param opt_type the opt type
         * @return true success
         * @return false fail
         */
        bool CheckFileStat(string& recipe_path, int opt_type);

        /**
         * @brief load previous stat 
         * 
         */
        void LoadStat();

        /**
         * @brief store current stat
         * 
         */
        void StoreStat();

        /**
         * @brief print the info of curClient
         * 
         * @param total_cache_size total cache size
         */
        void PrintClientLog(uint64_t total_cache_size);

    public:
        uint64_t _total_upload_opt_num = 0;
        uint64_t _total_download_opt_num = 0;

        /**
         * @brief Construct a new ServerOptThd object
         * 
         * @param server_channel the server channel
         * @param fp_2_addr_db the fp to address index
         * @param feature_2_fp_db the feature to base hash index
         */
        ServerOptThd(SSLConnection* server_channel, AbsDatabase* fp_2_addr_db, 
            AbsDatabase* feature_2_fp_db);

        /**
         * @brief Destroy the ServerOptThd object
         * 
         */
        ~ServerOptThd();

        /**
         * @brief the main thread
         * 
         * @param client_ssl 
         */
        void Run(SSL* client_ssl);
};

#endif