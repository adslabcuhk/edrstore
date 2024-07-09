/**
 * @file configure.h
 * @author Zuoru Yang
 * @brief define the necessary variables in deduplication
 * @version 0.1
 * @date 2021-8
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef EDRSTORE_CONFIGURE_H
#define EDRSTORE_CONFIGURE_H

#include <bits/stdc++.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "const_var.h"
#include "define.h"

using namespace std;

class Configure {
    private:
        string my_name_ = "Configure";
        // chunking settings
        uint64_t chunking_type_;
        uint64_t max_chunk_size_;
        uint64_t min_chunk_size_;
        uint64_t avg_chunk_size_;
        uint64_t chunker_sliding_win_size_;
        uint64_t read_size_; //128M per time

        // similar config 
        uint64_t similar_sliding_win_size_;

        // storage server settings
        string storage_server_ip_;
        int storage_server_port_;
        string recipe_root_path_;
        string container_root_path_;
        string fp_2_chunk_db_;
        string feature_2_fp_db_;
        uint64_t container_cache_size_;

        // key manager settings
        string km_ip_;
        int km_port_;
        string feature_2_key_db_;

        // client settings
        uint32_t client_id_;
        uint64_t send_chunk_batch_size_;
        uint64_t send_recipe_batch_size_;
        string user_key_;

        // const 
        string recipe_suffix_ = "-recipe";
        string container_suffix_ = "-container";
        string key_recipe_suffix_ = "-key";

        /**
         * @brief parse the json file
         * 
         * @param path the path to the json file
         */
        void ReadConfig(string path);

    public:

        /**
         * @brief Construct a new Configure object
         * 
         * @param path the path to the json file
         */
        Configure(string path);

        /**
         * @brief Destroy the Configure object
         * 
         */
        ~Configure();

        // chunking settings
        uint64_t GetChunkingType() {
            return chunking_type_;
        }
        uint64_t GetMaxChunkSize() {
            return max_chunk_size_;
        }
        uint64_t GetMinChunkSize() {
            return min_chunk_size_;
        }
        uint64_t GetAvgChunkSize() {
            return avg_chunk_size_;
        }
        uint64_t GetChunkerSlidingWinSize() {
            return chunker_sliding_win_size_;
        }
        uint64_t GetReadSize() {
            return read_size_;
        }

        // similar detection setting
        uint64_t GetSimilarSlidingWinSize() {
            return similar_sliding_win_size_;
        }

        // storage management settings
        string GetStorageServerIP() {
            return storage_server_ip_;
        }
        int GetStorageServerPort() {
            return storage_server_port_;
        }
        string GetRecipeRootPath() {
            return recipe_root_path_;
        }
        string GetContainerRootPath() {
            return container_root_path_;
        }
        string GetFp2ChunkDBName() {
            return fp_2_chunk_db_;
        }
        string GetFeature2FpDBName() {
            return feature_2_fp_db_;
        }
        uint64_t GetContainerCacheSize() {
            return container_cache_size_;
        }

        // key management settings
        string GetKeyServerIP() {
            return km_ip_;
        }
        int GetKeyServerPort() {
            return km_port_;
        }
        string GetFeature2KeyDBName() {
            return feature_2_key_db_;
        }

        // client settings
        uint32_t GetClientID() {
            return client_id_;
        }
        uint64_t GetSendChunkBatchSize() {
            return send_chunk_batch_size_;
        }
        uint64_t GetSendRecipeBatchSize() {
            return send_recipe_batch_size_;
        }
        string GetUserKey() {
            return user_key_;
        }

        // global
        string GetRecipeSuffix() {
            return recipe_suffix_;
        }
        string GetContainerSuffix() {
            return container_suffix_;
        }
        string GetKeyRecipeSuffix() {
            return key_recipe_suffix_;
        }
};

#endif