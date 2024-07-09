/**
 * @file storage_core.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of StorageCore
 * @version 0.1
 * @date 2022-07-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_STORAGE_CORE
#define EDRSTORE_STORAGE_CORE

#include "../configure.h"
#include "client_var.h"

class StorageCore {
    private:
        string my_name_ = "StorageCore";

        // for config
        string container_name_prefix_;
        string container_name_suffix_;

    public:
        uint64_t _read_chunk_num = 0;
        uint64_t _read_data_size = 0;
        uint64_t _read_container_num = 0;

        uint64_t _write_chunk_num = 0;
        uint64_t _write_data_size = 0;
        uint64_t _write_container_num = 0;

        /**
         * @brief Construct a new StorageCore object
         * 
         */
        StorageCore();

        /**
         * @brief Destroy the StorageCore object
         * 
         */
        ~StorageCore();

        /**
         * @brief read a chunk
         * 
         * @param addr the chunk address
         * @param data the chunk data
         * @param cur_client the current client
         */
        // void ReadChunk(const KeyForChunkHashDB_t* addr, uint8_t* data,
        //     ClientVar* cur_client);
        bool ReadChunk(const KeyForChunkHashDB_t* addr, uint8_t* data,
            ClientVar* cur_client);

        /**
         * @brief write a chunk
         * 
         * @param addr the chunk address
         * @param data the chunk data
         * @param size the chunk size
         * @param cur_client the current client
         */
        void WriteChunk(KeyForChunkHashDB_t* addr, uint8_t* data, uint32_t size,
            ClientVar* cur_client);

        /**
         * @brief write the container to the file system
         * 
         * @param new_container new container
         */
        void SaveContainer(Container_t* new_container);
};

#endif