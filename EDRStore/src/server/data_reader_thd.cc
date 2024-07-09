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
    delta_comp_ = new DeltaComp();
}

/**
 * @brief Destroy the DataReaderThd object
 * 
 */
DataReaderThd::~DataReaderThd() {
    delete delta_comp_;
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
            
            // cout<<"read file recipe fp: "<<endl;
            // tool::PrintBinaryArray(tmp_fp, CHUNK_HASH_SIZE);

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
    switch (addr->stat) {
        case COMP_DELTA_CHUNK: {
            // it is a similar chunk
            this->ProcNormalDeltaChunk(addr, &raw_chunk, cur_client);
            break;
        }
        case COMP_BASE_CHUNK: {
            // it is a normal base chunk, directly pass
            break;
        }
        case CACHE_DELTA_CHUNK: {
            // it is a similar chunk base on the cache
            this->ProcCacheDeltaChunk(addr, &raw_chunk, cur_client);
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "");
            exit(EXIT_FAILURE);
        }
    }

    // step-3: insert chunk to the MQ
    cur_client->_reader_2_decoder_mq->Push(raw_chunk);

    return ;
}

/**
 * @brief process normal delta chunk
 * 
 * @param input_addr input address
 * @param raw_chunk input raw chunk <ret>
 * @param cur_client current client
 */
void DataReaderThd::ProcNormalDeltaChunk(KeyForChunkHashDB_t* input_addr,
    Reader2Decoder_t* raw_chunk, ClientVar* cur_client) {
    string base_addr_str;
    if (!fp_2_addr_db_->QueryBuffer((char*)input_addr->base_fp, CHUNK_HASH_SIZE,
        base_addr_str)) {
        tool::Logging(my_name_.c_str(), "cannot find the req base chunk addr.\n");
        exit(EXIT_FAILURE);
    }

    // read its base chunk
    KeyForChunkHashDB_t* base_addr = (KeyForChunkHashDB_t*)&base_addr_str[0];
    storage_core_->ReadChunk(base_addr, raw_chunk->base_chunk.data, cur_client);
    raw_chunk->base_chunk.header.size = base_addr->len;
    raw_chunk->base_chunk.header.type = base_addr->stat;

    return ;
}

/**
 * @brief process cache delta chunk
 * 
 * @param input_addr input address
 * @param raw_chunk input raw chunk
 * @param cur_client current client
 */
void DataReaderThd::ProcCacheDeltaChunk(KeyForChunkHashDB_t* input_addr,
    Reader2Decoder_t* raw_chunk, ClientVar* cur_client) {
    InformCache* inform_cache = cur_client->_inform_cache;
    
    // first check the cache
    if (inform_cache->IsBaseChunkExist(input_addr->base_fp)) {
        // it exists, directly read from cache
        raw_chunk->base_chunk.header.size =
            inform_cache->FetchBaseChunk(input_addr->base_fp,
                raw_chunk->base_chunk.data);
        raw_chunk->base_chunk.header.type = UNCOMP_BASE_CHUNK;
    } else {
        // it not exists, query the "compressed base chunk"
        string base_addr_str;
        if (!fp_2_addr_db_->QueryBuffer((char*)input_addr->base_fp, CHUNK_HASH_SIZE,
            base_addr_str)) {
            tool::Logging(my_name_.c_str(), "cannot find the req base chunk addr.\n");
            exit(EXIT_FAILURE);
        }

        // read its base chunk
        KeyForChunkHashDB_t* base_addr = (KeyForChunkHashDB_t*)&base_addr_str[0];
        switch (base_addr->stat) {
            case COMP_DELTA_CHUNK: {
                // it is a delta base chunk, read the base chunk in the input_chunk
                Reader2Decoder_t tmp_base_chunk;
                storage_core_->ReadChunk(base_addr, tmp_base_chunk.input_chunk.data,
                    cur_client);
                tmp_base_chunk.input_chunk.header.size = base_addr->len;
                tmp_base_chunk.input_chunk.header.type = base_addr->stat;

                // read the base chunk of the base chunk
                string base_base_addr_str;
                if (!fp_2_addr_db_->QueryBuffer((char*)base_addr->base_fp, CHUNK_HASH_SIZE,
                    base_base_addr_str)) {
                    tool::Logging(my_name_.c_str(),
                        "cannot find the req base base chunk addr.\n");
                    exit(EXIT_FAILURE);
                }

                KeyForChunkHashDB_t* base_base_addr =
                    (KeyForChunkHashDB_t*)&base_base_addr_str[0];
                storage_core_->ReadChunk(base_base_addr, tmp_base_chunk.base_chunk.data,
                    cur_client);
                tmp_base_chunk.base_chunk.header.size = base_base_addr->len;
                tmp_base_chunk.base_chunk.header.type = base_base_addr->stat;

                // do delta decoding to restore the compressed base chunk 
                raw_chunk->base_chunk.header.size = delta_comp_->DeltaDecode(
                    tmp_base_chunk.base_chunk.data,
                    tmp_base_chunk.base_chunk.header.size,
                    tmp_base_chunk.input_chunk.data,
                    tmp_base_chunk.input_chunk.header.size,
                    raw_chunk->base_chunk.data);
                raw_chunk->base_chunk.header.type = COMP_BASE_CHUNK;
                break;
            }
            case COMP_BASE_CHUNK: {
                // directly read the base chunk
                storage_core_->ReadChunk(base_addr, raw_chunk->base_chunk.data, cur_client);
                raw_chunk->base_chunk.header.size = base_addr->len;
                raw_chunk->base_chunk.header.type = COMP_BASE_CHUNK;
                break;
            }
            case CACHE_DELTA_CHUNK: {
                // TODO: this means multi-level delta should avoid
                raw_chunk->base_chunk.header.type = MULTI_LEVEL_DELTA_CHUNK;
                break;
            }
            default: {
                tool::Logging(my_name_.c_str(),
                    "wrong base chunk type for cache delta chunk: %u.\n",
                    base_addr->stat);
                exit(EXIT_FAILURE);
            }
        } 
    }

    return ;
}