/**
 * @file plain_similar_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of PlainSimilarThd
 * @version 0.1
 * @date 2022-06-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/plain_similar_thd.h"

/**
 * @brief Construct a new PlainSimilarThd object
 * 
 */
PlainSimilarThd::PlainSimilarThd() {
    rabin_util_ = new RabinFPUtil(config.GetSimilarSlidingWinSize());
    finesse_util_ = new FinesseUtil(SUPER_FEATURE_PER_CHUNK,    
        FEATURE_PER_CHUNK, FEATURE_PER_SUPER_FEATURE);
    rabin_util_->NewCtx(rabin_ctx_);
}

/**
 * @brief Destroy the PlainSimilarThd object
 * 
 */
PlainSimilarThd::~PlainSimilarThd() {
    rabin_util_->FreeCtx(rabin_ctx_);
    delete rabin_util_;
    delete finesse_util_;
}

/**
 * @brief the main thread
 * 
 * @param input_MQ the input MQ
 * @param output_MQ the output MQ
 */
void PlainSimilarThd::Run(AbsMQ<Chunk_t>* input_MQ,
    AbsMQ<FeatureChunk_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    FeatureChunk_t tmp_data;
    while (true) {
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data.chunk)) {
            // extract a chunk from the MQ
            gettimeofday(&proc_stime, NULL);
            switch(tmp_data.chunk.type) {
                case NORMAL_CHUNK: {
                    finesse_util_->ExtractFeature(rabin_ctx_, tmp_data.chunk.raw_chunk.data,
                        tmp_data.chunk.raw_chunk.size, tmp_data.features);
                    break;
                }
                case RECIPE_CHUNK: {
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);

            // insert the chunk to the output MQ
            output_MQ->Push(tmp_data);
        }
    }

    // set the output MQ done
    output_MQ->_done = true;
    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total proc time: %lf, "
        "total running time: %lf\n", total_proc_time, total_running_time);

    return ;
}