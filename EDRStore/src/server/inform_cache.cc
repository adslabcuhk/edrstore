/**
 * @file inform_cache.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of InformCache
 * @version 0.1
 * @date 2022-08-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/inform_cache.h"

/**
 * @brief Construct a new InformCache object
 * 
 * @param client_id client id
 */
InformCache::InformCache(uint32_t client_id) {
    client_id_ = client_id;
    cache_root_path_ = config.GetCacheRootPath();
    this->LoadCntIdx();

    DatabaseFactory db_factory;
    string db_path = cache_root_path_ + to_string(client_id_) + "_db";
    base_2_data_db_ = db_factory.CreateDatabase(ROCKSDB_DB,
        db_path);
    
    finesse_util_ = new FinesseUtil(SUPER_FEATURE_PER_CHUNK,
        FEATURE_PER_CHUNK, FEATURE_PER_SUPER_FEATURE);
    rabin_util_ = new RabinFPUtil(config.GetSimilarSlidingWinSize());
    rabin_util_->NewCtx(rabin_ctx_);

    similar_policy_ = new SimilarPolicy();

    delta_comp_ = new DeltaComp(); 
    cache_base_chunk_str_.reserve(ENC_MAX_CHUNK_SIZE);
}

/**
 * @brief Destroy the InformCache object
 * 
 */
InformCache::~InformCache() {
    this->StoreCntIdx();
    delete base_2_data_db_;
    delete finesse_util_;
    rabin_util_->FreeCtx(rabin_ctx_);
    delete similar_policy_;
    delete delta_comp_;
    delete rabin_util_;
}

/**
 * @brief process normal chunk
 * 
 * @param cache_chunk cache chunk
 */
void InformCache::InsertCachedChunk(WrappedChunk_t* cache_chunk) {
    // update the local feature index
    string base_fp_str;
    base_fp_str.assign((char*)cache_chunk->info.fp, CHUNK_HASH_SIZE);    

    similar_policy_->UpdateFeatureIndex(local_feature_2_fp_db_,
        cache_chunk->info.features, base_fp_str);
    
    if (base_2_cnt_idx_.find(base_fp_str) != base_2_cnt_idx_.end()) {
        base_2_cnt_idx_[base_fp_str].first += SUPER_FEATURE_PER_CHUNK;
    } else {
        // insert to the kv-store
        base_2_cnt_idx_[base_fp_str] = 
            {SUPER_FEATURE_PER_CHUNK, cache_chunk->info.size};
        cache_base_chunk_str_.assign((char*)cache_chunk->data,
            cache_chunk->info.size);
        base_2_data_db_->Insert(base_fp_str, cache_base_chunk_str_);
    }

    return ;
}

/**
 * @brief process normal chunk
 * 
 * @param input_chunk input chunk
 * @param output_chunk output chunk
 * @return true perform delta compression
 * @return false cannot perform delta compression
 */
bool InformCache::ProcessNormalChunk(WrappedChunk_t* input_chunk,
    WrappedChunk_t* output_chunk) {
    similar_policy_->FindBaseChunk(local_feature_2_fp_db_,
        &input_chunk->info);
    bool ret = false;

    switch (input_chunk->info.stat) {
        case SIMILAR_CHUNK: {
            // fetch the base chunk
            if (base_2_data_db_->QueryBuffer(
                (char*)input_chunk->info.addr.base_fp,
                CHUNK_HASH_SIZE, cache_base_chunk_str_)) {
                output_chunk->info.size = delta_comp_->DeltaEncode(
                    (uint8_t*)&cache_base_chunk_str_[0],
                    cache_base_chunk_str_.size(), input_chunk->data, 
                    input_chunk->info.size, output_chunk->data);

                // copy the metadata to the input chunk 
                memcpy(output_chunk->info.addr.base_fp,
                    input_chunk->info.addr.base_fp, CHUNK_HASH_SIZE);
                memcpy(output_chunk->info.fp, input_chunk->info.fp,
                    CHUNK_HASH_SIZE);
                output_chunk->info.stat = CACHE_DELTA_CHUNK;
            } else {
                tool::Logging(my_name_.c_str(), "cannot find the base "
                    "chunk in the inform cache.\n");
                exit(EXIT_FAILURE);
            }
            ret = true;
            break;
        }
        case NON_SIMILAR_CHUNK: {
            ret = false;
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong chunk type "
                "in inform cache.\n");
            exit(EXIT_FAILURE);
        }
    }
    return ret;
}

/**
 * @brief load cnt index
 * 
 */
void InformCache::LoadCntIdx() {
    string cnt_idx_path = cache_root_path_ + to_string(client_id_) + "_cnt";
    // check cache meta data exist
    ifstream cache_cnt_hdl;
    if (tool::FileExist(cnt_idx_path)) {
        // check the file size
        cache_cnt_hdl.open(cnt_idx_path, ios_base::in | ios_base::binary);
        if (!cache_cnt_hdl.is_open()) {
            tool::Logging(my_name_.c_str(), "cannot open the cache cnt.\n");
            exit(EXIT_FAILURE);
        }

        size_t start_size = cache_cnt_hdl.tellg();
        cache_cnt_hdl.seekg(0, ios_base::end);
        size_t file_size = cache_cnt_hdl.tellg();
        file_size = file_size - start_size;

        if (file_size == 0) {
            return ;
        }
        cache_cnt_hdl.seekg(0, ios_base::beg);

        // it exists in the client, read it
        size_t idx_item_num = 0;
        cache_cnt_hdl.read((char*)&idx_item_num, sizeof(size_t));
        if (idx_item_num == 0) {
            return ;
        }

        string base_fp_str;
        base_fp_str.resize(CHUNK_HASH_SIZE, 0);
        uint32_t cnt;
        uint32_t chunk_size;
        for (size_t i = 0; i < idx_item_num; i++) {
            cache_cnt_hdl.read((char*)&base_fp_str[0], CHUNK_HASH_SIZE);
            cache_cnt_hdl.read((char*)&cnt, sizeof(uint32_t));
            cache_cnt_hdl.read((char*)&chunk_size, sizeof(uint32_t));
            base_2_cnt_idx_[base_fp_str] = {cnt, chunk_size};
        }

        cache_cnt_hdl.close();
    }

    // -------- load local feature index --------
    // check local feature index exits
    ifstream feature_idx_hdl;
    string feature_idx_path = cache_root_path_ + to_string(client_id_) +
        "_idx";
    if (tool::FileExist(feature_idx_path)) {
        feature_idx_hdl.open(feature_idx_path, ios_base::in | ios_base::binary);
        if (!feature_idx_hdl.is_open()) {
            tool::Logging(my_name_.c_str(),
                "cannot open the cache feature index.\n");
            exit(EXIT_FAILURE);
        }

        size_t start_size = feature_idx_hdl.tellg();
        feature_idx_hdl.seekg(0, ios_base::end);
        size_t file_size = feature_idx_hdl.tellg();
        file_size = file_size - start_size;

        if (file_size == 0) {
            return ;
        }
        feature_idx_hdl.seekg(0, ios_base::beg);

        size_t feature_item_num = 0;
        feature_idx_hdl.read((char*)&feature_item_num, sizeof(size_t));
        if (feature_item_num == 0) {
            return ;
        }

        string tmp_base_fp;
        tmp_base_fp.resize(CHUNK_HASH_SIZE, 0);
        uint64_t feature;
        for (size_t i = 0; i < feature_item_num; i++) {
            feature_idx_hdl.read((char*)&feature, sizeof(uint64_t));
            feature_idx_hdl.read(&tmp_base_fp[0], CHUNK_HASH_SIZE);
            local_feature_2_fp_db_[feature] = tmp_base_fp;
        }

        feature_idx_hdl.close();
    }

    return ;
}

/**
 * @brief store cnt index
 * 
 */
void InformCache::StoreCntIdx() {
    string cnt_idx_path = cache_root_path_ + to_string(client_id_) + "_cnt";
    ofstream cache_cnt_hdl;
    cache_cnt_hdl.open(cnt_idx_path, ios_base::trunc | ios_base::binary);
    if (!cache_cnt_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init the cache cnt idx.\n");
        exit(EXIT_FAILURE);
    }

    size_t idx_item_num = base_2_cnt_idx_.size();
    cache_cnt_hdl.write((char*)&idx_item_num, sizeof(size_t));

    for (auto it : base_2_cnt_idx_) {
        cache_cnt_hdl.write(it.first.c_str(), CHUNK_HASH_SIZE);
        cache_cnt_hdl.write((char*)&it.second.first, sizeof(uint32_t));
        cache_cnt_hdl.write((char*)&it.second.second, sizeof(uint32_t));
    }
    cache_cnt_hdl.close();

    // -------- store local feature index --------
    string feature_idx_path = cache_root_path_ + to_string(client_id_) +
        "_idx";
    ofstream feature_idx_hdl;
    feature_idx_hdl.open(feature_idx_path, ios_base::trunc | ios_base::binary);
    if (!feature_idx_hdl.is_open()) {
        tool::Logging(my_name_.c_str(),
            "cannot init the cache feature index.\n");
        exit(EXIT_FAILURE);
    }

    size_t feature_item_num = local_feature_2_fp_db_.size();
    feature_idx_hdl.write((char*)&feature_item_num, sizeof(size_t));

    for (auto it : local_feature_2_fp_db_) {
        feature_idx_hdl.write((char*)&it.first, sizeof(uint64_t));
        feature_idx_hdl.write(it.second.c_str(),CHUNK_HASH_SIZE);
    }
    feature_idx_hdl.close();

    return ;
}

/**
 * @brief process evict chunk
 * 
 * @param evict_chunk evict_chunk
 */
void InformCache::EvictCacheChunk(WrappedChunk_t* evict_chunk) {
    uint32_t feature_num = evict_chunk->info.size;
    uint64_t* feature_ptr;
    string base_fp_str;

    for (size_t i = 0; i < feature_num; i++) {
        feature_ptr = (uint64_t*)(evict_chunk->data + i * sizeof(uint64_t));
        // check base chunk hash
        auto find_base_ret = local_feature_2_fp_db_.find(*feature_ptr);
        if (find_base_ret != local_feature_2_fp_db_.end()) {
            auto find_cnt_ret = base_2_cnt_idx_.find(find_base_ret->second);
            if (find_cnt_ret != base_2_cnt_idx_.end()) {
                if (find_cnt_ret->second.first > 0) {
                    find_cnt_ret->second.first--;
                } else {
                    find_cnt_ret->second.first = 0;
                }
            } else {
                tool::Logging(my_name_.c_str(),
                    "cannot find the evict chunk in count index.\n");
                exit(EXIT_FAILURE);
            }

            local_feature_2_fp_db_.erase(*feature_ptr);
        } else {
            tool::Logging(my_name_.c_str(), "cannot find the evict feature"
                "in local feature index.\n");
            exit(EXIT_FAILURE);
        }
    }

    return ;
}

/**
 * @brief delete evict chunk
 * 
 * @return total cache size
 */
uint64_t InformCache::DeleteEvictChunk() {
    uint64_t total_cache_size = 0;
    auto it = base_2_cnt_idx_.begin();
    while (it != base_2_cnt_idx_.end()) {
        if (it->second.first == 0) {
            base_2_data_db_->Delete(it->first);
            it = base_2_cnt_idx_.erase(it);
        } else {
            total_cache_size += it->second.second;
            it++;
        }
    }

    return total_cache_size;
}


/**
 * @brief check if req base chunk is exist
 * 
 * @param base_fp base chunk fp
 * @return true exist
 * @return false not exist
 */
bool InformCache::IsBaseChunkExist(uint8_t* base_fp) {
    string base_fp_str;
    base_fp_str.assign((char*)base_fp, CHUNK_HASH_SIZE);
    if (base_2_cnt_idx_.find(base_fp_str) != base_2_cnt_idx_.end()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief fetch base chunk from cache
 * 
 * @param base_fp base chunk fp
 * @param output_base output base chunk <ret>
 * @return uint32_t output base chunk size
 */
uint32_t InformCache::FetchBaseChunk(uint8_t* base_fp, uint8_t* output_base) {
    string base_fp_str;
    base_fp_str.assign((char*)base_fp, CHUNK_HASH_SIZE);
    if (!base_2_data_db_->Query(base_fp_str, cache_base_chunk_str_)) {
        tool::Logging(my_name_.c_str(), "cannot find the base chunk in the cache.\n");
        exit(EXIT_FAILURE);
    }
    uint32_t base_chunk_size = cache_base_chunk_str_.size();
    memcpy(output_base, cache_base_chunk_str_.c_str(), base_chunk_size);
    return base_chunk_size;
}

/**
 * @brief Get the Cache Size object
 * 
 * @return uint64_t the cache size
 */
uint64_t InformCache::GetCacheSize() {
    return local_feature_2_fp_db_.size();
}