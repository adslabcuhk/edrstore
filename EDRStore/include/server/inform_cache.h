/**
 * @file inform_cache.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces 
 * @version 0.1
 * @date 2022-08-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_INFORM_CACHE_H
#define EDRSTORE_INFORM_CACHE_H

#include "../configure.h"
#include "../data_structure.h"
#include "../database/db_factory.h"
#include "../chunker/finesse_util.h"
#include "../reduction/similar_policy.h"
#include "../reduction/delta_comp.h"

extern Configure config;

class InformCache {
    private:
        string my_name_ = "InformCache";
        uint32_t client_id_;
        string cache_root_path_;

        // feature to base fp
        AbsDatabase* base_2_data_db_; // LevelDB

        // record the reference count <cnt, chunk size>
        unordered_map<string, pair<uint32_t,uint32_t>> base_2_cnt_idx_;
        unordered_map<uint64_t, string> local_feature_2_fp_db_;

        // for feature computation
        FinesseUtil* finesse_util_;

        // rabin ctx
        RabinCtx_t rabin_ctx_;
        RabinFPUtil* rabin_util_;

        // for similar detection
        SimilarPolicy* similar_policy_;

        // for delta compression
        DeltaComp* delta_comp_;
        string cache_base_chunk_str_;

        /**
         * @brief load cnt index
         * 
         */
        void LoadCntIdx();

        /**
         * @brief store cnt index
         * 
         */
        void StoreCntIdx();

    public:
        /**
         * @brief Construct a new InformCache object
         * 
         * @param client_id client id
         */
        InformCache(uint32_t client_id);

        /**
         * @brief Destroy the InformCache object
         * 
         */
        ~InformCache();

        /**
         * @brief process normal chunk
         * 
         * @param input_chunk input chunk
         * @param output_chunk output chunk
         * @return true perform delta compression
         * @return false cannot perform delta compression
         */
        bool ProcessNormalChunk(WrappedChunk_t* input_chunk,
            WrappedChunk_t* output_chunk);
        
        /**
         * @brief process normal chunk
         * 
         * @param cache_chunk cache chunk
         */
        void InsertCachedChunk(WrappedChunk_t* cache_chunk);

        /**
         * @brief process evict chunk
         * 
         * @param evict_chunk evict_chunk
         */
        void EvictCacheChunk(WrappedChunk_t* evict_chunk);

        /**
         * @brief delete evict chunk
         * 
         * @return total cache size
         */
        uint64_t DeleteEvictChunk();

        /**
         * @brief fetch base chunk from cache
         * 
         * @param base_fp base chunk fp
         * @param output_base output base chunk <ret>
         * @return uint32_t output base chunk size
         */
        uint32_t FetchBaseChunk(uint8_t* base_fp, uint8_t* output_base);

        /**
         * @brief check if req base chunk is exist
         * 
         * @param base_fp base chunk fp
         * @return true exist
         * @return false not exist
         */
        bool IsBaseChunkExist(uint8_t* base_fp);

        /**
         * @brief Get the Cache Size object
         * 
         * @return uint64_t the cache size
         */
        uint64_t GetCacheSize();
};

#endif