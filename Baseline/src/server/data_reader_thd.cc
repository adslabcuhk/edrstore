/**
 * @file data_reader_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DataReaderThd
 * @version 0.1
 * @date 2022-07-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/data_reader_thd.h"

/**
 * @brief Construct a new DataReaderThd object
 * 
 * @param fp_2_addr_db fp to chunk addr index
 * @param storage_core storage core
 */
DataReaderThd::DataReaderThd(AbsDatabase* fp_2_addr_db, StorageCore* storage_core) {
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();
    fp_2_addr_db_ = fp_2_addr_db;
    storage_core_ = storage_core;
}

/**
 * @brief Destroy the DataReaderThd object
 * 
 */
DataReaderThd::~DataReaderThd() {

}

/**
 * @brief the main process
 * 
 * @param cur_client current client
 */
void DataReaderThd::Run(ClientVar* cur_client) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");
    uint8_t* read_recipe_buf = cur_client->_read_recipe_buf;
    ifstream* read_recipe_hdl = &cur_client->_recipe_read_hdl;

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    bool is_end = false; 
    while (!is_end) {
        // step-1: read file recipe
        gettimeofday(&proc_stime, NULL);
        read_recipe_hdl->read((char*)read_recipe_buf, send_recipe_batch_size_ * CHUNK_HASH_SIZE);
        size_t read_item_num = read_recipe_hdl->gcount() / CHUNK_HASH_SIZE;
        if (read_item_num == 0) {
            // directly break
            break;
        }
        is_end = read_recipe_hdl->eof();

        uint8_t* tmp_fp;
        for (size_t i = 0; i < read_item_num; i++) {
            tmp_fp = read_recipe_buf + CHUNK_HASH_SIZE * i;
            this->FetchChunk(tmp_fp, cur_client);
        }
        gettimeofday(&proc_etime, NULL);
        total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);
    }
    cur_client->_reader_2_decoder_mq->_done = true;

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total proc time: %lf. "
        "total running time: %lf\n", total_proc_time, total_running_time);

    return ;
}

/**
 * @brief fetch a chunk (also its base chunk if necessary)
 * 
 * @param fp chunk fp
 * @param cur_client current client
 */
void DataReaderThd::FetchChunk(uint8_t* fp, ClientVar* cur_client) {
    string addr_str;
    if (!fp_2_addr_db_->QueryBuffer((char*)fp, CHUNK_HASH_SIZE, addr_str)) {
        tool::Logging(my_name_.c_str(), "cannot find the req chunk addr.\n");
        exit(EXIT_FAILURE);
    }

    // step-1: read the chunk
    KeyForChunkHashDB_t* addr = (KeyForChunkHashDB_t*)&addr_str[0];
    Reader2Decoder_t raw_chunk;
    storage_core_->ReadChunk(addr, raw_chunk.input_chunk.data, cur_client);
    raw_chunk.input_chunk.header.size = addr->len;
    raw_chunk.input_chunk.header.type = addr->stat;

    // step-2: check if it is similar chunk
    if ((addr->stat == UNCOMP_DELTA_CHUNK) || (addr->stat == COMP_DELTA_CHUNK)) {
        // it is a similar chunk
        string base_addr_str;
        if (!fp_2_addr_db_->QueryBuffer((char*)addr->base_fp, CHUNK_HASH_SIZE,
            base_addr_str)) {
            tool::Logging(my_name_.c_str(), "cannot find the req base chunk addr.\n");
            exit(EXIT_FAILURE);
        }

        // read its base chunk
        KeyForChunkHashDB_t* base_addr = (KeyForChunkHashDB_t*)&base_addr_str[0]; 
        storage_core_->ReadChunk(base_addr, raw_chunk.base_chunk.data, cur_client);
        raw_chunk.base_chunk.header.size = base_addr->len;
        raw_chunk.base_chunk.header.type = base_addr->stat;
    }
    
    // step-3: insert chunk to the MQ
    cur_client->_reader_2_decoder_mq->Push(raw_chunk);

    return ;
}