/**
 * @file dedup_detect.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of deduplication detection
 * @version 0.1
 * @date 2022-07-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DEDUP_DETECT_H
#define EDRSTORE_DEDUP_DETECT_H

#include "../database/db_factory.h"
#include "../configure.h"
#include "../data_structure.h"

class DedupDetect {
    private:
        string my_name_ = "DedupDetect";
        
        // for the dedup index
        AbsDatabase* fp_2_addr_db_;

    public:
        /**
         * @brief Construct a new DedupDetect object
         * 
         * @param fp_2_addr_db the deduplication index
         */
        DedupDetect(AbsDatabase* fp_2_addr_db);

        /**
         * @brief Destroy the DedupDetect object
         * 
         */
        ~DedupDetect();

        /**
         * @brief detect deduplicate chunks
         * 
         * @param info the stat
         */
        void DetectDuplicate(ChunkInfo_t* info);
};

#endif