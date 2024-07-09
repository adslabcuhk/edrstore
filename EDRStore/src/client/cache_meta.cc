/**
 * @file cache_meta.cc
 * @author Zhao Jia
 * @brief 
 * @version 0.1
 * @date 2022-06-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/cache_meta.h"

/**
 * @brief Construct a new CacheMeta object
 * 
 * @param server_channel the connection to the storage server
 * @param server_conn_record the storage server connection record
 */
CacheMeta::CacheMeta(SSLConnection* server_channel,
    pair<int, SSL*> server_conn_record) {
    this->LoadCacheMeta();
    cur_version_num_++;
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();
    client_id_ = config.GetClientID();

    // init the evict feature send buf
    evict_feature_buf_.send_buf = (uint8_t*) malloc(send_recipe_batch_size_ * 
        sizeof(uint64_t) + sizeof(NetworkHead_t));
    evict_feature_buf_.header = (NetworkHead_t*) evict_feature_buf_.send_buf;
    evict_feature_buf_.header->client_id = client_id_;
    evict_feature_buf_.header->size = 0;
    evict_feature_buf_.header->cur_item_num = 0;
    evict_feature_buf_.data_buf = evict_feature_buf_.send_buf + sizeof(NetworkHead_t);

    // for storage server connection
    server_channel_ = server_channel;
    server_conn_record_ = server_conn_record;
    server_ssl_ = server_conn_record.second;
}

/**
 * @brief Destroy the CacheMeta object
 * 
 */
CacheMeta::~CacheMeta(){
    this->StoreCacheMeta();
    free(evict_feature_buf_.send_buf);
}

/**
 * @brief load cache metadata
 * 
 */
void CacheMeta::LoadCacheMeta() {
    // check cache meta data exist
    ifstream cache_meta_hdl;
    if (tool::FileExist(db_name_)) {
        // check the file size
        cache_meta_hdl.open(db_name_, ios_base::in | ios_base::binary);
        if (!cache_meta_hdl.is_open()) {
            tool::Logging(my_name_.c_str(), "cannot open the cache meta.\n");
            exit(EXIT_FAILURE);
        }

        size_t start_size = cache_meta_hdl.tellg();
        cache_meta_hdl.seekg(0, ios_base::end);
        size_t file_size = cache_meta_hdl.tellg();
        file_size = file_size - start_size;

        if (file_size == 0) {
            return ;
        }
        cache_meta_hdl.seekg(0, ios_base::beg);

        cache_meta_hdl.read((char*)&_total_similar_chunk, sizeof(uint64_t));
        cache_meta_hdl.read((char*)&cur_version_num_, sizeof(uint32_t));

        // it exists in the client, read it
        size_t idx_item_num = 0;
        cache_meta_hdl.read((char*)&idx_item_num, sizeof(size_t));
        if (idx_item_num == 0) {
            return ;
        }

        uint64_t feature;
        uint32_t version_num;
        for (size_t i = 0; i < idx_item_num; i++) {
            cache_meta_hdl.read((char*)&feature, sizeof(uint64_t));
            cache_meta_hdl.read((char*)&version_num, sizeof(uint32_t));
            feature_2_version_idx_[feature] = version_num;
        }

        cache_meta_hdl.close();
    }

    return ;
}

/**
 * @brief store cache metadata
 * 
 */
void CacheMeta::StoreCacheMeta() {
    ofstream cache_meta_hdl;
    cache_meta_hdl.open(db_name_, ios_base::trunc | ios_base::binary);
    if (!cache_meta_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init the cache meta.\n");
        exit(EXIT_FAILURE);
    }

    cache_meta_hdl.write((char*)&_total_similar_chunk, sizeof(uint64_t));
    cache_meta_hdl.write((char*)&cur_version_num_, sizeof(uint32_t));
    
    size_t idx_item_num = feature_2_version_idx_.size();
    cache_meta_hdl.write((char*)&idx_item_num, sizeof(size_t));

    // cache meta index mapping: feature -> current version
    for (auto it : feature_2_version_idx_) {
        cache_meta_hdl.write((char*)&it.first, sizeof(uint64_t));
        cache_meta_hdl.write((char*)&it.second, sizeof(uint32_t));
    }
    cache_meta_hdl.close();

    return ;
}

/**
 * @brief insert the features to the cache
 * 
 * @param features input features
 */
void CacheMeta::UpdateCacheMeta(uint64_t* features) {
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        feature_2_version_idx_[features[i]] = cur_version_num_;
    }
    return ;
}

/**
 * @brief query the cache meta to check whether it is similar?
 * 
 * @param features input features
 * @return true it is similar chunk
 * @return false it is non-similar chunk
 */
bool CacheMeta::QueryCacheMeta(uint64_t* features) {
    bool is_similar = false;
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        if (feature_2_version_idx_.find(features[i]) !=
            feature_2_version_idx_.end()) {
            // it exist in the index, update the version
            feature_2_version_idx_[features[i]] = cur_version_num_;
            is_similar = true;
        }
    }
    if (is_similar) {
        _total_similar_chunk++;
    }

    return is_similar;
}

/**
 * @brief evict feature based on last_n_para_
 * 
 */
void CacheMeta::EvictFeature() {
    auto it = feature_2_version_idx_.begin();

    while (it != feature_2_version_idx_.end()) {
        if (cur_version_num_ - it->second >= last_n_para_) {
            memcpy(
                evict_feature_buf_.data_buf + evict_feature_buf_.header->size, 
                &it->first, sizeof(uint64_t));
            evict_feature_buf_.header->size += sizeof(uint64_t);
            evict_feature_buf_.header->cur_item_num++;

            // check whether to send evict feature buffer
            if (evict_feature_buf_.header->cur_item_num %
                send_recipe_batch_size_ == 0) {
                this->SendFeatures();
            }

            // erasure the feature    
            it = feature_2_version_idx_.erase(it);
        } else {
            it++;
        }
    }
    
    // Process the tail
    if (evict_feature_buf_.header->cur_item_num != 0) {
        this->SendFeatures();
    }

    return ;
}

/**
 * @brief send a batch of features
 * 
 */
void CacheMeta::SendFeatures() {
    evict_feature_buf_.header->msg_type = CLIENT_UPLOAD_FEATURE;
    if (!server_channel_->SendData(server_ssl_,
        evict_feature_buf_.send_buf,
        evict_feature_buf_.header->size + sizeof(NetworkHead_t))) {
        tool::Logging(my_name_.c_str(), "send the feature batch error.\n");
        exit(EXIT_FAILURE);
    }

    // clear the current evict feature buffer
    evict_feature_buf_.header->cur_item_num = 0;
    evict_feature_buf_.header->size = 0;

    return ;
}