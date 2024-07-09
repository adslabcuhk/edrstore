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
    AbsMQ<Chunk_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");
    bool is_end = false;

    Chunk_t tmp_data;
    while (!is_end) {
        uint64_t pending_size = 0;
        pending_size = chunker_obj_->LoadDataFromFile(*input_file_hdl);
        if (pending_size == 0) {
            break;
        }

        while (true) {

#ifdef EDR_BREAKDOWN
            gettimeofday(&_chunking_stime, NULL);
#endif

            tmp_data.raw_chunk.size = chunker_obj_->GenerateOneChunk(tmp_data.raw_chunk.data);
        
#ifdef EDR_BREAKDOWN
            gettimeofday(&_chunking_etime, NULL);
            _total_chunking_time += tool::GetTimeDiff(_chunking_stime,
                _chunking_etime);
            _total_chunking_data_size += tmp_data.raw_chunk.size;
#endif

            if (tmp_data.raw_chunk.size == 0) {
                break;
            }

#ifdef EDR_BREAKDOWN
            gettimeofday(&_fp_stime, NULL);
#endif

            crypto_util_->GenerateHash(md_ctx_, tmp_data.raw_chunk.data,
                tmp_data.raw_chunk.size, tmp_data.raw_chunk.fp);
            
#ifdef EDR_BREAKDOWN
            gettimeofday(&_fp_etime, NULL);
            _total_fp_time += tool::GetTimeDiff(_fp_stime, _fp_etime);
            _total_fp_data_size += tmp_data.raw_chunk.size;
#endif

            tmp_data.type = NORMAL_CHUNK;             
            output_MQ->Push(tmp_data);
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