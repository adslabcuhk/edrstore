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
#ifdef EDR_BREAKDOWN
            gettimeofday(&_plain_feature_stime, NULL);
#endif

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

#ifdef EDR_BREAKDOWN
            gettimeofday(&_plain_feature_etime, NULL);
            _total_plain_feature_time += tool::GetTimeDiff(
                _plain_feature_stime, _plain_feature_etime);
            if (tmp_data.chunk.type != RECIPE_CHUNK) {
                _total_plain_feature_size += tmp_data.chunk.raw_chunk.size;
            }
#endif

            // insert the chunk to the output MQ
            output_MQ->Push(tmp_data);
        }
    }

    // set the output MQ done
    output_MQ->_done = true;
    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total running time: %lf\n",
        total_running_time);

    return ;
}