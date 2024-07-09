/**
 * @file data_writer_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of 
 * @version 0.1
 * @date 2022-07-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef DATA_WRITER_THD_H
#define DATA_WRITER_THD_H

#include "../configure.h"
#include "../database/db_factory.h"
#include "../reduction/delta_comp.h"
#include "../reduction/similar_policy.h"
#include "../compression/compress_util.h"
#include "client_var.h"
#include "storage_core.h"

class DataWriterThd {
    private:
        string my_name_ = "DataWriterThd";
    
        // storage core
        StorageCore* storage_core_;

        // for delta compression
        DeltaComp* delta_comp_;

        // for similar detection
        SimilarPolicy* similar_policy_;

        // for fp to chunk addr index
        AbsDatabase* fp_2_addr_db_;
        AbsDatabase* feature_2_fp_db_;

        /**
         * @brief process a similar chunk
         * 
         * @param input_chunk input chunk
         * @param cur_client current client
         */
        void ProcSimilarChunk(WrappedChunk_t* input_chunk, ClientVar* cur_client);

        /**
         * @brief process a non-similar chunk 
         * 
         * @param input_chunk input chunk
         * @param cur_client current client
         */
        void ProcNonSimilarChunk(WrappedChunk_t* input_chunk, ClientVar* cur_client);

        /**
         * @brief process a cache delta chunk
         * 
         * @param input_chunk input chunk
         * @param cur_client current client
         */
        void ProcCacheDeltaChunk(WrappedChunk_t* input_chunk, ClientVar* cur_client);

        /**
         * @brief fetch the base chunk
         * 
         * @param base_fp base chunk fp
         * @param base_data base chunk data
         * @param cur_client current client
         * @return uint32_t base chunk size
         */
        uint32_t FetchBaseChunk(uint8_t* base_fp, uint8_t* base_data,
            ClientVar* cur_client);

    public:
        // for delta compression
        uint64_t _total_similar_chunk_num = 0;
        uint64_t _total_similar_data_size = 0;
        uint64_t _total_delta_size = 0;

#ifdef EDR_BREAKDOWN
        struct timeval _comp_delta_stime;
        struct timeval _comp_delta_etime;
        double _total_comp_delta_time = 0;
        uint64_t _total_comp_delta_data_size = 0;
#endif

        /**
         * @brief Construct a new DataWriterThd object
         * 
         * @param fp_2_addr_db fp to chunk addr index
         * @param feature_2_fp_db feature to fp index
         * @param storage_core storage core
         */
        DataWriterThd(AbsDatabase* fp_2_addr_db, AbsDatabase* feature_2_fp_db,
            StorageCore* storage_core);

        /**
         * @brief Destroy the DataWriterThd object
         * 
         */
        ~DataWriterThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client var
         */
        void Run(ClientVar* cur_client);
};

#endif