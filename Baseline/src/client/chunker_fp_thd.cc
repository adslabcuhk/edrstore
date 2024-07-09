/**
 * @file chunker_fp_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of ChunkerFPThd
 * @version 0.1
 * @date 2022-06-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/chunker_fp_thd.h"

/**
 * @brief Construct a new ChunkerFPThd object
 * 
 */
ChunkerFPThd::ChunkerFPThd() {
    crypto_util_ = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
    ChunkerFactory chunk_factory;
    chunker_obj_ = chunk_factory.CreateChunker(config.GetChunkingType());
    md_ctx_ = EVP_MD_CTX_new(); 
}

ChunkerFPThd::~ChunkerFPThd() {
    delete crypto_util_;
    delete chunker_obj_;
    EVP_MD_CTX_free(md_ctx_);
}

/**
 * @brief the main thread
 * 
 * @param input_file_hdl the input file handler
 * @param output_MQ the output MQ
 */
void ChunkerFPThd::Run(ifstream* input_file_hdl,
    AbsMQ<Chunker2KeyGen_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    Chunk_t tmp_data;
    while (true) {
        uint64_t pending_size = 0;
        pending_size = chunker_obj_->LoadDataFromFile(*input_file_hdl);
        if (pending_size == 0) {
            break;
        }

        while (true) {
#if (CHUNKING_BREAKDOWN == 1)    
            gettimeofday(&chunking_stime_, NULL);
#endif
            tmp_data.raw_chunk.size = chunker_obj_->GenerateOneChunk(tmp_data.raw_chunk.data);
#if (CHUNKING_BREAKDOWN == 1)
            gettimeofday(&chunking_etime_, NULL);
            total_chunking_time_ += tool::GetTimeDiff(chunking_stime_, chunking_etime_);
#endif

#if (CHUNKING_BREAKDOWN == 1)
            gettimeofday(&fp_stime_, NULL);
#endif
            if (!tmp_data.raw_chunk.size) {
                break;
            } else {
                crypto_util_->GenerateHash(md_ctx_, tmp_data.raw_chunk.data,
                    tmp_data.raw_chunk.size, tmp_data.raw_chunk.fp);
                tmp_data.type = NORMAL_CHUNK;             
                output_MQ->Push(tmp_data);
            }
#if (CHUNKING_BREAKDOWN == 1)
            gettimeofday(&fp_etime_, NULL);
            total_fp_time_ += tool::GetTimeDiff(fp_stime_, fp_etime_);
#endif
        }
    }

    tmp_data.type = RECIPE_CHUNK;
    tmp_data.head.chunk_num = chunker_obj_->_total_chunk_num;
    tmp_data.head.size = chunker_obj_->_total_file_size;
    output_MQ->Push(tmp_data);
    output_MQ->_done = true;

    _total_chunk_num = chunker_obj_->_total_chunk_num;
    _total_file_size = chunker_obj_->_total_file_size;

    tool::Logging(my_name_.c_str(), "total file size (B): %lu\n", chunker_obj_->_total_file_size); 
    tool::Logging(my_name_.c_str(), "total chunk num: %lu\n", chunker_obj_->_total_chunk_num); 
#if (CHUNKING_BREAKDOWN == 1)
    tool::Logging(my_name_.c_str(), "total chunking time: %lf\n", total_chunking_time_); 
    tool::Logging(my_name_.c_str(), "total fp time: %lf\n", total_fp_time_); 
#endif
    return ;
}