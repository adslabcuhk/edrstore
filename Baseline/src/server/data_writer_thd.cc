/**
 * @file data_writer_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DataWriterThd 
 * @version 0.1
 * @date 2022-07-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/data_writer_thd.h"

/**
 * @brief Construct a new DataWriterThd object
 * 
 * @param fp_2_addr_db fp to chunk addr index
 * @param feature_2_fp_db for feature to fp index
 * @param storage_core storage core
 */
DataWriterThd::DataWriterThd(AbsDatabase* fp_2_addr_db,
    AbsDatabase* feature_2_fp_db, StorageCore* storage_core) {
    fp_2_addr_db_ = fp_2_addr_db;
    feature_2_fp_db_ = feature_2_fp_db;
    storage_core_ = storage_core;
    delta_comp_ = new DeltaComp();
    similar_policy_ = new SimilarPolicy();
    compress_util_ = new CompressUtil(COMPRESSION_TYPE, COMPRESSION_LEVEL);
}

/**
 * @brief Destroy the DataWriterThd object
 * 
 */
DataWriterThd::~DataWriterThd() {
    delete delta_comp_;
    delete similar_policy_;
    delete compress_util_;
}

/**
 * @brief the main process
 * 
 * @param cur_client current client var
 */
void DataWriterThd::Run(ClientVar* cur_client) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");
    AbsMQ<WrappedChunk_t>* input_MQ = cur_client->_recv_2_writer_mq;

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    RabinCtx_t rabin_ctx;
    RabinFPUtil* rabin_util = new RabinFPUtil(config.GetSimilarSlidingWinSize());
    rabin_util->NewCtx(rabin_ctx);

    gettimeofday(&stime, NULL);
    // -------- main process --------
    WrappedChunk_t tmp_data;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data)) {
            // check the base chunk
            gettimeofday(&proc_stime, NULL);
            similar_policy_->FindBaseChunk(feature_2_fp_db_,
                &tmp_data.info);
            switch (tmp_data.info.stat) {
                case SIMILAR_CHUNK: {
                    _total_similar_chunk_num++;
                    _total_similar_data_size += tmp_data.info.addr.len;
                    this->ProcSimilarChunk(&tmp_data, cur_client);
                    break;
                }
                case NON_SIMILAR_CHUNK: {
                    this->ProcNonSimilarChunk(&tmp_data, cur_client);
                    similar_policy_->UpdateFeatureIndex(
                        feature_2_fp_db_,
                        tmp_data.info.features,
                        tmp_data.info.fp
                    );
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
                    exit(EXIT_FAILURE);
                }
            }

            // update the fp index
            fp_2_addr_db_->InsertBothBuffer((char*)tmp_data.info.fp, CHUNK_HASH_SIZE,
                (char*)&tmp_data.info.addr, sizeof(KeyForChunkHashDB_t));

            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);
        }
    }

    // check the tail container
    if (cur_client->_cur_container.cur_size != 0) {
        storage_core_->SaveContainer(&cur_client->_cur_container);
    }

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total proc time: %lf, "
        "total running time: %lf\n", total_proc_time, total_running_time);

    rabin_util->FreeCtx(rabin_ctx);
    delete rabin_util;

    return ;
}

/**
 * @brief process a similar chunk
 * 
 * @param input_chunk input chunk
 * @param cur_client current client
 */
void DataWriterThd::ProcSimilarChunk(WrappedChunk_t* input_chunk,
    ClientVar* cur_client) {
    uint8_t base_chunk[ENC_MAX_CHUNK_SIZE];
    uint32_t base_chunk_size = 0;
    uint8_t delta_chunk[ENC_MAX_CHUNK_SIZE];
    uint32_t delta_chunk_size = 0;

    base_chunk_size = this->FetchBaseChunk(input_chunk->info.addr.base_fp,
        base_chunk, cur_client);
    delta_chunk_size = delta_comp_->DeltaEncode(base_chunk, base_chunk_size,
        input_chunk->data, input_chunk->info.addr.len, delta_chunk);
    
    // update stat
    _total_delta_size += delta_chunk_size;

    uint32_t compressed_size = 0;
    if (compress_util_->CompressOneChunk(delta_chunk, delta_chunk_size,
        input_chunk->data, &compressed_size)) {
        // can compress
        storage_core_->WriteChunk(&input_chunk->info.addr, input_chunk->data,
            compressed_size, cur_client);
        input_chunk->info.addr.stat = COMP_DELTA_CHUNK;
    } else {
        // cannot compress
        storage_core_->WriteChunk(&input_chunk->info.addr, input_chunk->data,
            compressed_size, cur_client);
        input_chunk->info.addr.stat = UNCOMP_DELTA_CHUNK;
    }

    return ; 
}

/**
 * @brief process a non-similar chunk 
 * 
 * @param input_chunk input chunk
 * @param cur_client current client
 */
void DataWriterThd::ProcNonSimilarChunk(WrappedChunk_t* input_chunk,
    ClientVar* cur_client) {
    uint8_t compressed_chunk[ENC_MAX_CHUNK_SIZE];
    uint32_t compressed_size = 0;

    if (compress_util_->CompressOneChunk(input_chunk->data,
        input_chunk->info.addr.len, compressed_chunk, &compressed_size)) {
        // can compress
        storage_core_->WriteChunk(&input_chunk->info.addr, compressed_chunk,
            compressed_size, cur_client);
        input_chunk->info.addr.stat = COMP_BASE_CHUNK;
    } else {
        // cannot compress
        storage_core_->WriteChunk(&input_chunk->info.addr, compressed_chunk,
            compressed_size, cur_client);
        input_chunk->info.addr.stat = UNCOMP_BASE_CHUNK;
    }

    return ;
}

/**
 * @brief fetch the base chunk
 * 
 * @param base_fp base chunk fp
 * @param base_data base chunk data
 * @param cur_client current client
 * @return uint32_t base chunk size
 */
uint32_t DataWriterThd::FetchBaseChunk(uint8_t* base_fp, uint8_t* base_data,
    ClientVar* cur_client) {
    string base_addr_str;

    // step-1: query the fp index to get the base chunk address
    if (!fp_2_addr_db_->QueryBuffer((char*)base_fp, CHUNK_HASH_SIZE, base_addr_str)) {
        tool::Logging(my_name_.c_str(), "req base chunk not exits.\n");
        exit(EXIT_FAILURE);
    }

    // step-2: read base chunk from the disk
    uint8_t raw_base_chunk[ENC_MAX_CHUNK_SIZE];
    KeyForChunkHashDB_t* base_addr = (KeyForChunkHashDB_t*)&base_addr_str[0];
    storage_core_->ReadChunk(base_addr, raw_base_chunk, cur_client);

    // step-3: perform decompression if necessary
    uint32_t base_size = 0;
    switch (base_addr->stat) {
        case COMP_BASE_CHUNK: {
            compress_util_->DecompressOneChunk(raw_base_chunk,
                base_addr->len, base_data, &base_size);
            break;
        }
        case UNCOMP_BASE_CHUNK: {
            memcpy(base_data, raw_base_chunk, base_addr->len);
            base_size = base_addr->len;
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong base chunk type.\n");
            exit(EXIT_FAILURE);
        }
    }

    return base_size;
}