/**
 * @file cache_meta.h
 * @author Zhao Jia
 * @brief 
 * @version 0.1
 * @date 2022-06-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_CACHE_META_H
#define EDRSTORE_CACHE_META_H

#include "../configure.h"
#include "../define.h"
#include "../crypto/crypto_util.h"
#include "../network/ssl_conn.h"

using namespace std;

extern Configure config;

class CacheMeta{
    private:
        string my_name_= "CacheMeta";
        string db_name_ = "cache_meta_db";

        uint32_t cur_version_num_ = 0;

        uint64_t send_recipe_batch_size_ = 0;
        uint32_t client_id_ = 0;

        // feature --> version
        unordered_map<uint64_t, uint32_t> feature_2_version_idx_;

        uint32_t last_n_para_ = 1;

        // evict feature buf
        SendMsgBuffer_t evict_feature_buf_;

        // for communication
        SSLConnection* server_channel_;
        pair<int, SSL*> server_conn_record_;
        SSL* server_ssl_;

        /**
         * @brief load cache metadata
         * 
         */
        void LoadCacheMeta();

        /**
         * @brief store cache metadata
         * 
         */
        void StoreCacheMeta();

        /**
         * @brief send a batch of features
         * 
         */
        void SendFeatures();

    public:
        // record current cache size
        uint64_t _total_similar_chunk = 0; 

        /**
         * @brief Construct a new CacheMeta object
         * 
         * @param server_channel the connection to the storage server
         * @param server_conn_record the storage server connection record
         */
        CacheMeta(SSLConnection* server_channel,
            pair<int, SSL*> server_conn_record);

        /**
         * @brief Destroy the CacheMeta object
         * 
         */
        ~CacheMeta();

        /**
         * @brief insert the features to the cache
         * 
         * @param features input features
         */
        void UpdateCacheMeta(uint64_t* features);

        /**
         * @brief query the cache meta to check whether it is similar?
         * 
         * @param features input features
         * @return true it is similar chunk
         * @return false it is non-similar chunk
         */
        bool QueryCacheMeta(uint64_t* features);

        /**
         * @brief evict feature based on last_n_para_
         * 
         */
        void EvictFeature();

        /**
         * @brief Get the Feature Num object
         * 
         * @return size_t the num of feature
         */
        size_t GetFeatureNum() {
            return feature_2_version_idx_.size();
        }
};

#endif