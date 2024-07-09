/**
 * @file configure.cc
 * @author Zhao Jia
 * @brief implement interfaces defined in configure
 * @version 0.1
 * @date 2021-8
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "configure.h"

/**
 * @brief Construct a new Configure object
 * 
 * @param path the input configure file path
 */
Configure::Configure(string path){
    this->ReadConfig(path);
}

/**
 * @brief Destory the Configure object
 * 
 */
Configure::~Configure(){}

/**
 * @brief Read setting data from json file
 * 
 * @param path the input configure file path
 */
/**
 * @brief parse the json file
 * 
 * @param path the path to the json file
 */
void Configure::ReadConfig(string path) {
    using namespace boost;
    using namespace boost::property_tree;
    ptree root;
    read_json<ptree>(path, root);

    // Chunker settings
    chunking_type_ = root.get<uint64_t>("Chunker.chunking_type");
    max_chunk_size_ = root.get<uint64_t>("Chunker.max_chunk_size");
    min_chunk_size_ = root.get<uint64_t>("Chunker.min_chunk_size");
    avg_chunk_size_ = root.get<uint64_t>("Chunker.avg_chunk_size");
    chunker_sliding_win_size_ = root.get<uint64_t>("Chunker.sliding_win_size");
    read_size_ = root.get<uint64_t>("Chunker.read_size");

    // Similar detection setting
    similar_sliding_win_size_ = root.get<uint64_t>("Similar.sliding_win_size");

    // Storage Server settings
    storage_server_ip_ = root.get<string>("StorageServer.ip");
    storage_server_port_ = root.get<int>("StorageServer.port");
    recipe_root_path_ = root.get<string>("StorageServer.recipe_root_path");
    container_root_path_ = root.get<string>("StorageServer.container_root_path");
    fp_2_chunk_db_ = root.get<string>("StorageServer.fp_2_chunk_db");
    feature_2_fp_db_ = root.get<string>("StorageServer.feature_2_fp_db");
    container_cache_size_ = root.get<uint64_t>("StorageServer.container_cache_size");

    // key manager settings
    km_ip_ = root.get<string>("KeyServer.ip");
    km_port_ = root.get<int>("KeyServer.port");
    feature_2_key_db_ = root.get<string>("KeyServer.feature_2_key_db");

    // client settings
    client_id_ = root.get<uint32_t>("Client.id");
    send_chunk_batch_size_ = root.get<uint64_t>("Client.send_chunk_batch_size");
    send_recipe_batch_size_ = root.get<uint64_t>("Client.send_recipe_batch_size");
    user_key_ = root.get<string>("Client.user_key");

    if (send_recipe_batch_size_ % send_chunk_batch_size_ != 0) {
        tool::Logging(my_name_.c_str(), "recipe batch size should be a multiple "
            "of chunk batch size.\n");
        exit(EXIT_FAILURE);
    }

    return ;
}