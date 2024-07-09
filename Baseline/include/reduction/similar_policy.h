/**
 * @file similar_policy.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of SimilarPolicy
 * @version 0.1
 * @date 2022-08-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_SIMILAR_POLICY
#define EDRSTORE_SIMILAR_POLICY

#include "../database/db_factory.h"
#include "../data_structure.h"

class SimilarPolicy {
    private:
        string my_name_ = "SimilarPolicy";

    public:
        /**
         * @brief Construct a new SimilarPolicy object
         * 
         */
        SimilarPolicy();

        /**
         * @brief Destroy the SimilarPolicy object
         * 
         */
        ~SimilarPolicy();

        /**
         * @brief find the base chunk
         * 
         * @param feature_2_fp_db feature to base fp index
         * @param info chunk info
         */
        void FindBaseChunk(AbsDatabase* feature_2_fp_db,
            ChunkInfo_t* info);

        /**
         * @brief find the base chunk
         * 
         * @param feature_2_fp_db feature to base fp index
         * @param info chunk info
         */
        void FindBaseChunk(unordered_map<uint64_t, string>& feature_2_fp_db,
            ChunkInfo_t* info);

        /**
         * @brief update the feature index 
         * 
         * @param feature_2_fp_db feature to base fp index
         * @param features chunk feature
         * @param base_fp base chunk fp
         */
        void UpdateFeatureIndex(AbsDatabase* feature_2_fp_db,
            uint64_t* features, uint8_t* base_fp);

        /**
         * @brief update the feature index
         * 
         * @param feature_2_fp_db feature to base fp index
         * @param features chunk feature
         * @param base_fp base chunk fp
         */
        void UpdateFeatureIndex(unordered_map<uint64_t, string>& feature_2_fp_db,
            uint64_t* features, string& base_fp);
};

#endif