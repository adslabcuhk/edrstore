/**
 * @file chunker_fp_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of ChunkerFPThd
 * @version 0.1
 * @date 2022-06-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_CHUNKER_FP_THD_H
#define EDRSTORE_CHUNKER_FP_THD_H

#include "../define.h"
#include "../configure.h"
#include "../chunker/chunker_factory.h"
#include "../chunker/abs_chunker.h"
#include "../crypto/crypto_util.h"
#include "../message_queue/mq_factory.h"
#include "../data_structure.h"

using namespace std;

extern Configure config;

class ChunkerFPThd {
    private:
        string my_name_ = "ChunkerFPThd";

        AbsChunker* chunker_obj_;

        CryptoUtil* crypto_util_;
        EVP_MD_CTX* md_ctx_;

    public:
        uint64_t _total_file_size = 0;
        uint64_t _total_chunk_num = 0;

#ifdef EDR_BREAKDOWN
        struct timeval _chunking_stime;
        struct timeval _chunking_etime;
        double _total_chunking_time = 0;
        uint64_t _total_chunking_data_size = 0; 

        struct timeval _fp_stime;
        struct timeval _fp_etime;
        double _total_fp_time = 0;
        uint64_t _total_fp_data_size = 0;
#endif

        /**
         * @brief Construct a new ChunkerFPThd
         * 
         */
        ChunkerFPThd();

        /**
         * @brief Destroy the ChunkerFPThd
         * 
         */
        ~ChunkerFPThd();

        /**
         * @brief the main thread
         * 
         * @param input_file_hdl the input file handler
         * @param output_MQ the output MQ
         */
        void Run(ifstream* input_file_hdl, AbsMQ<Chunk_t>* output_MQ);
};

#endif