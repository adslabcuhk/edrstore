/**
 * @file similar_policy.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of SimilarPolicy
 * @version 0.1
 * @date 2022-08-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/reduction/similar_policy.h"

bool CmpPair(pair<string, uint32_t>& a, 
    pair<string, uint32_t>& b) {
    return a.second > b.second;
}

/**
 * @brief Construct a new SimilarPolicy object
 * 
 */
SimilarPolicy::SimilarPolicy() {

}

/**
 * @brief Destroy the SimilarPolicy object
 * 
 */
SimilarPolicy::~SimilarPolicy() {

}

/**
 * @brief find the base chunk
 * 
 * @param feature_2_fp_db feature to base fp index
 * @param info chunk info
 */
void SimilarPolicy::FindBaseChunk(AbsDatabase* feature_2_fp_db,
    ChunkInfo_t* info) {
    // query the feature index to get the base chunk hash
    unordered_map<string, uint32_t> feature_freq_map;
    bool is_similar = false;
    bool is_first_match = true;
    uint8_t first_match_base_fp[CHUNK_HASH_SIZE];
    string tmp_base_fp;
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        // count the freq of each base key
        if (feature_2_fp_db->QueryBuffer((char*)&info->features[i],
            sizeof(uint64_t), tmp_base_fp)) {
            if (is_first_match) {
                memcpy(first_match_base_fp, tmp_base_fp.c_str(),
                    CHUNK_HASH_SIZE);
                is_first_match = false;
            }

            if (feature_freq_map.find(tmp_base_fp) != feature_freq_map.end()) {
                feature_freq_map[tmp_base_fp]++;
            } else {
                feature_freq_map[tmp_base_fp] = 1;
            }
            is_similar = true;
        }
    }

    if (is_similar) {
        // similar chunk
        // find the most similar base chunk
        vector<pair<string, uint32_t>> tmp_freq_vec;
        for (auto it : feature_freq_map) {
            tmp_freq_vec.push_back(it);
        }
        sort(tmp_freq_vec.begin(), tmp_freq_vec.end(), CmpPair);
        if (tmp_freq_vec[0].second == 1) {
            memcpy(info->addr.base_fp, first_match_base_fp, CHUNK_HASH_SIZE);
        } else {
            memcpy(info->addr.base_fp, tmp_freq_vec[0].first.c_str(),
                CHUNK_HASH_SIZE);
        }
        info->stat = SIMILAR_CHUNK;
    } else {
        info->stat = NON_SIMILAR_CHUNK;
    }
    
    return ;
}

/**
 * @brief find the base chunk
 * 
 * @param feature_2_fp_db feature to base fp index
 * @param info chunk info
 */
void SimilarPolicy::FindBaseChunk(
    unordered_map<uint64_t, string>& feature_2_fp_db,
    ChunkInfo_t* info) {
    // query the feature index to get the base chunk hash
    unordered_map<string, uint32_t> feature_freq_map;
    bool is_similar = false;
    bool is_first_match = true;
    uint8_t first_match_base_fp[CHUNK_HASH_SIZE];
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        // count the freq of each base key
        auto find_base_ret = feature_2_fp_db.find(info->features[i]);
        if (find_base_ret != feature_2_fp_db.end()) {
            if (is_first_match) {
                memcpy(first_match_base_fp, find_base_ret->second.c_str(),
                    CHUNK_HASH_SIZE);
                is_first_match = false;
            }
            auto find_freq_ret = feature_freq_map.find(find_base_ret->second);
            if (find_freq_ret != feature_freq_map.end()) {
                find_freq_ret->second++;
            } else {
                feature_freq_map[find_base_ret->second] = 1;
            }
            is_similar = true;
        }
    }

    if (is_similar) {
        // similar chunk
        // find the most similar base chunk
        vector<pair<string, uint32_t>> tmp_freq_vec;
        for (auto it : feature_freq_map) {
            tmp_freq_vec.push_back(it);
        }
        sort(tmp_freq_vec.begin(), tmp_freq_vec.end(), CmpPair);
        if (tmp_freq_vec[0].second == 1) {
            memcpy(info->addr.base_fp, first_match_base_fp, CHUNK_HASH_SIZE);
        } else {
            memcpy(info->addr.base_fp, tmp_freq_vec[0].first.c_str(),
                CHUNK_HASH_SIZE);
        }
        info->stat = SIMILAR_CHUNK;
    } else {
        info->stat = NON_SIMILAR_CHUNK;
    }
    
    return ;
}

/**
 * @brief update the feature index 
 * 
 * @param feature_2_fp_db feature to base fp index
 * @param features chunk feature
 * @param base_fp base chunk fp
 */
void SimilarPolicy::UpdateFeatureIndex(AbsDatabase* feature_2_fp_db,
    uint64_t* features, uint8_t* base_fp) {
    // non-similar chunk, update the feature index
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        feature_2_fp_db->InsertBothBuffer((char*)&features[i],
            sizeof(uint64_t), (char*)base_fp, CHUNK_HASH_SIZE);
    }
    return ;
}

/**
 * @brief update the feature index
 * 
 * @param feature_2_fp_db feature to base fp index
 * @param features chunk feature
 * @param base_fp base chunk fp
 */
void SimilarPolicy::UpdateFeatureIndex(
    unordered_map<uint64_t, string>& feature_2_fp_db,
    uint64_t* features, string& base_fp) {
    // non-similar chunk, update the feature index
    for (size_t i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
        feature_2_fp_db[features[i]] = base_fp;
    }
    return ;
}