/**
 * @file data_reader_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DataReaderThd
 * @version 0.1
 * @date 2022-07-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DATA_READER_THD_H
#define EDRSTORE_DATA_READER_THD_H

#include "client_var.h"
#include "storage_core.h"
#include "../database/db_factory.h"
#include "../configure.h"
#include "../reduction/delta_comp.h"

extern Configure config;

class DataReaderThd {
    private:
        string my_name_ = "DataReaderThd";

        // for config
        uint64_t send_recipe_batch_size_;

        // for storage core
        StorageCore* storage_core_;

        // for fp index
        AbsDatabase* fp_2_addr_db_;

        // for delta decoding
        DeltaComp* delta_comp_;
        
        /**
         * @brief fetch a chunk (also its base chunk if necessary)
         * 
         * @param fp chunk fp
         * @param cur_client current client
         */
        void FetchChunk(uint8_t* fp, ClientVar* cur_client);

        /**
         * @brief process normal delta chunk
         * 
         * @param input_addr input address
         * @param raw_chunk input raw chunk <ret>
         * @param cur_client current client
         */
        void ProcNormalDeltaChunk(KeyForChunkHashDB_t* input_addr,
            Reader2Decoder_t* raw_chunk, ClientVar* cur_client);

        /**
         * @brief process cache delta chunk
         * 
         * @param input_addr input address
         * @param raw_chunk input raw chunk
         * @param cur_client current client
         */
        void ProcCacheDeltaChunk(KeyForChunkHashDB_t* input_addr,
            Reader2Decoder_t* raw_chunk, ClientVar* cur_client);

    public:
        /**
         * @brief Construct a new DataReaderThd object
         * 
         * @param fp_2_addr_db fp to chunk addr index
         * @param storage_core storage core
         */
        DataReaderThd(AbsDatabase* fp_2_addr_db, StorageCore* storage_core);

        /**
         * @brief Destroy the DataReaderThd object
         * 
         */
        ~DataReaderThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client
         */
        void Run(ClientVar* cur_client);
};

#endif