/**
 * @file storage_core.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of StorageCore
 * @version 0.1
 * @date 2022-07-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/storage_core.h"

/**
 * @brief Construct a new StorageCore object
 * 
 */
StorageCore::StorageCore() {
    container_name_prefix_ = config.GetContainerRootPath();
    container_name_suffix_ = config.GetContainerSuffix();
}

/**
 * @brief Destroy the StorageCore object
 * 
 */
StorageCore::~StorageCore() {

}

/**
 * @brief read a chunk
 * 
 * @param addr the chunk address
 * @param data the chunk data
 * @param cur_client the current client
 */
bool StorageCore::ReadChunk(const KeyForChunkHashDB_t* addr, uint8_t* data,
    ClientVar* cur_client) {
    ReadCache* container_cache = cur_client->_container_cache;

    // step-1: check container cache
    string req_name;
    req_name.assign((char*)addr->container_id, CONTAINER_ID_LENGTH);

    if (container_cache->Exist(req_name)) {
        // exist in the cache
        uint8_t* container_buf = container_cache->Read(req_name);
        memcpy(data, container_buf + addr->offset, addr->len);
    } else {
        // not exist in the cache, read it from disk
        ifstream container_file_hdl;
        string full_container_name = container_name_prefix_ + req_name +
            container_name_suffix_;
        if (!tool::FileExist(full_container_name)) {
            // check whether it is current container buffer
            Container_t* cur_container = &cur_client->_cur_container;
            if (strncmp(cur_container->id, req_name.c_str(), CONTAINER_ID_LENGTH) == 0) {
                // it is the current container buf
                memcpy(data, cur_container->body + addr->offset, addr->len);
            } else {
                // tool::Logging(my_name_.c_str(), "cannot find the container: %s\n",
                //     full_container_name.c_str());
                // exit(EXIT_FAILURE);
                return false;
            }
        } else {
            container_file_hdl.open(full_container_name, ios_base::in |
                ios_base::binary);
            // get the container size
            container_file_hdl.seekg(0, ios_base::end);
            int64_t container_size = container_file_hdl.tellg();
            container_file_hdl.seekg(0, ios_base::beg);

            // read container into the cache buffer
            Container_t tmp_container;
            container_file_hdl.read((char*)tmp_container.body, container_size);
            if (container_file_hdl.gcount() != container_size) {
                tool::Logging(my_name_.c_str(), "read size (%lu) cannot match "
                    "the expected container size (%lu).\n",
                    container_file_hdl.gcount(), container_size);
                exit(EXIT_FAILURE);
            }

            // close the container
            container_file_hdl.close();

            // read chunk from the tmp container buf
            memcpy(data, tmp_container.body + addr->offset, addr->len);

            // add the container to the cache
            container_cache->Insert(req_name, tmp_container.body, container_size);

            _read_container_num++;
        }
    }
    
    _read_chunk_num++;
    _read_data_size += addr->len;

    return true;
}

/**
 * @brief write a chunk
 * 
 * @param addr the chunk address
 * @param data the chunk data
 * @param size the chunk size
 * @param cur_client the current client
 */
void StorageCore::WriteChunk(KeyForChunkHashDB_t* addr, uint8_t* data, uint32_t size,
    ClientVar* cur_client) {
    Container_t* cur_container = &cur_client->_cur_container;
    if ((cur_container->cur_size + size) > MAX_CONTAINER_SIZE) {
        // cannot write in this container buf
        // save the current container first
        this->SaveContainer(cur_container);

        // reset the current container buf
        cur_container->cur_size = 0;
        tool::CreateUUID(cur_container->id, CONTAINER_ID_LENGTH);

        memcpy(cur_container->body + cur_container->cur_size, data, size);
    } else {
        // directly write this chunk to the container buffer
        memcpy(cur_container->body + cur_container->cur_size, data, size);
    }

    // update the chunk address
    memcpy(addr->container_id, cur_container->id, CONTAINER_ID_LENGTH);
    addr->len = size;
    addr->offset = cur_container->cur_size;

    // update the container current size
    cur_container->cur_size += size;

    _write_chunk_num++;
    _write_data_size += size;

    return ;
}

/**
 * @brief write the container to the file system
 * 
 * @param new_container new container
 */
void StorageCore::SaveContainer(Container_t* new_container) {
    ofstream container_file_hdl;
    string req_name(new_container->id, CONTAINER_ID_LENGTH);
    string full_container_name = container_name_prefix_ + req_name + container_name_suffix_;
    container_file_hdl.open(full_container_name, ios_base::out | ios_base::binary);
    if (!container_file_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot open the container %s\n",
            full_container_name.c_str());
        exit(EXIT_FAILURE);
    }

    // write container data
    container_file_hdl.write((char*)new_container->body, new_container->cur_size);
    container_file_hdl.flush();
    container_file_hdl.close();

    _write_container_num++;
    return ;
}