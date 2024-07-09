/**
 * @file select_comp_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the implementation SelectCompThd
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/select_comp_thd.h"

/**
 * @brief Construct a new SelectCompThd object
 * 
 */
/**
 * @brief Construct a new SelectCompThd object
 * 
 * @param method_type method type
 */
SelectCompThd::SelectCompThd(uint32_t method_type) {
    comp_pad_ = new CompPad();
    method_type_ = method_type;

    // for crypto
    mle_util_ = new MLE();

    // for random padding
    srand(tool::GetStrongSeed());
}

/**
 * @brief Destroy the SelectCompThd object
 * 
 */
SelectCompThd::~SelectCompThd() {
    delete comp_pad_;
    delete mle_util_;
}

/**
 * @brief the main thread
 * 
 * @param input_MQ the input MQ
 * @param output_MQ the output MQ
 */
void SelectCompThd::Run(AbsMQ<KeyGen2SelectComp_t>* input_MQ,
    AbsMQ<SelectComp2Sender_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    EncFeatureChunk_t tmp_data;
    SelectComp2Sender_t tmp_send_chunk;
    uint64_t tmp_pad_seed; 
    uint32_t tmp_w_padding_size;
    uint8_t tmp_compressed_chunk[ENC_MAX_CHUNK_SIZE];
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data)) {
            // extract a chunk from the MQ
            gettimeofday(&proc_stime, NULL);
            switch (tmp_data.feature_chunk.chunk.type) {
                case NORMAL_CHUNK: {
                    switch (method_type_) {
                        case PLAIN: {
                            ;
                        }
                        case SERVER_AIDED_MLE: {
                            tmp_send_chunk.send_chunk.header.type = NORMAL_CHUNK;
                            tmp_send_chunk.send_chunk.header.size = tmp_data.enc_size;
                            memcpy(tmp_send_chunk.send_chunk.data, tmp_data.enc_data,
                                tmp_send_chunk.send_chunk.header.size);

                            // update the key recipe
                            memcpy(tmp_send_chunk.key_recipe.key, tmp_data.key, CHUNK_HASH_SIZE);
                            break;
                        }
                        case ENC_COMP_MLE: {
                            tmp_pad_seed = rand();
                            tmp_w_padding_size = comp_pad_->CompressWithPad(
                                tmp_data.feature_chunk.chunk.raw_chunk.data,
                                tmp_data.feature_chunk.chunk.raw_chunk.size,
                                tmp_compressed_chunk, tmp_pad_seed);

                            // pad the key with seed to simulate MLE 
                            memcpy(tmp_data.key + CHUNK_HASH_SIZE - sizeof(uint32_t),
                                &tmp_pad_seed, sizeof(uint32_t));

                            // enc the chunk
                            tmp_send_chunk.send_chunk.header.type = NORMAL_CHUNK;
                            tmp_send_chunk.send_chunk.header.size = tmp_w_padding_size;
                            mle_util_->EncChunk(tmp_compressed_chunk, tmp_w_padding_size,
                                tmp_data.key, tmp_send_chunk.send_chunk.data);

                            // update the key recipe
                            memcpy(tmp_send_chunk.key_recipe.key, tmp_data.key, CHUNK_HASH_SIZE);
                            break;
                        }
                        default: {
                            tool::Logging(my_name_.c_str(), "wrong method type.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                    break;
                }
                case RECIPE_CHUNK: {
                    tmp_send_chunk.send_chunk.header.type = RECIPE_CHUNK;
                    tmp_send_chunk.send_chunk.header.size = sizeof(FileRecipeHead_t);
                    memcpy(tmp_send_chunk.send_chunk.data, &tmp_data.feature_chunk.chunk.head,
                        sizeof(FileRecipeHead_t));
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);

            output_MQ->Push(tmp_send_chunk);
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