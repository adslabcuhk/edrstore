/**
 * @file dedup_detect.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DedupDetect
 * @version 0.1
 * @date 2022-07-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/reduction/dedup_detect.h"

/**
 * @brief Construct a new DedupDetect object
 * 
 * @param fp_2_addr_db the deduplication index
 */
DedupDetect::DedupDetect(AbsDatabase* fp_2_addr_db) {
    fp_2_addr_db_ = fp_2_addr_db;
}

/**
 * @brief Destroy the DedupDetect object
 * 
 */
DedupDetect::~DedupDetect() {
}

/**
 * @brief detect deduplicate chunks
 * 
 * @param info the stat
 */
void DedupDetect::DetectDuplicate(ChunkInfo_t* info) {
    // check the index
    string ret_val;
    if (!fp_2_addr_db_->QueryBuffer((char*)info->fp, CHUNK_HASH_SIZE,
        ret_val)) {
        
        // unique chunk
        info->stat = UNIQUE_CHUNK;

        // update the index with "virtual"
        fp_2_addr_db_->InsertBothBuffer((char*)info->fp, CHUNK_HASH_SIZE,
            (char*)&info->addr, sizeof(KeyForChunkHashDB_t));
    } else {
        info->stat = DUPLICATE_CHUNK;
        // // check the corresponding compressed_fp
        // KeyForChunkHashDB_t* addr = (KeyForChunkHashDB_t*)& ret_val[0];
        // if(strcmp((const char*)addr->compressed_fp, (const char*)info->addr.compressed_fp) != 0) {
        //     cout<<"====earzhao===="<<endl;
        //     tool::PrintBinaryArray(addr->compressed_fp, CHUNK_HASH_SIZE);
        //     tool::PrintBinaryArray(info->addr.compressed_fp, CHUNK_HASH_SIZE);
        //     info->stat = UNIQUE_CHUNK;
        // }
        // else{
        //     info->stat = DUPLICATE_CHUNK;
        // }
    }

    return ;
}