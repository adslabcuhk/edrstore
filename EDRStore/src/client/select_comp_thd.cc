/**
 * @file select_comp_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of selective compression thread
 * @version 0.1
 * @date 2022-06-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/select_comp_thd.h"

/**
 * @brief Construct a new SelectCompThd object
 * 
 * @param cache_meta cache meta data
 * @param method_type method type 
 */
SelectCompThd::SelectCompThd(CacheMeta* cache_meta,
    uint32_t method_type) {
    comp_pad_ = new CompPad();
    two_phase_enc_ = new TwoPhaseEnc();
    cache_meta_ = cache_meta;
    method_type_ = method_type;

    crypto_util_ = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
    md_ctx = EVP_MD_CTX_new();
}

/**
 * @brief Destroy the Select Comp Thd object
 * 
 */
SelectCompThd::~SelectCompThd() {
    delete comp_pad_;
    delete two_phase_enc_;
    delete crypto_util_;
    EVP_MD_CTX_free(md_ctx);
}

/**
 * @brief the main thread
 * 
 * @param input_MQ the input MQ
 */
void SelectCompThd::Run(AbsMQ<EncFeatureChunk_t>* input_MQ,
    AbsMQ<SelectComp2Sender_t>* output_MQ){
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process ---------

    // for input MQ
    EncFeatureChunk_t tmp_data;
    // for output MQ
    SelectComp2Sender_t tmp_send_chunk;
    SelectComp2Sender_t tmp_cache_chunk; // for cache chunk
    while(true){
        // extract a chunk from MQ
        if(input_MQ->_done && input_MQ->IsEmpty()){
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if(input_MQ->Pop(tmp_data)){
            // extract a chunk from the MQ
            switch (method_type_) {
                case ONLY_SIMILAR_ENC: {
                    this->OnlyEncMode(&tmp_data, &tmp_send_chunk);
                    break;
                }
                case SIMILAR_ENC_COMP: {
                    this->EncCompMode(&tmp_data, &tmp_send_chunk);
                    break;
                }
                case FULL_EDR: {
                    if (this->FullEDR(&tmp_data, &tmp_cache_chunk,
                        &tmp_send_chunk)) {
                        // need to add a cache chunk
                        output_MQ->Push(tmp_cache_chunk);
                    }
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong method type.\n");
                    exit(EXIT_FAILURE);
                }
            }

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

/**
 * @brief only enc mode
 * 
 * @param input_chunk input chunk
 * @param output_chunk output chunk
 */
void SelectCompThd::OnlyEncMode(EncFeatureChunk_t* input_chunk,
    SelectComp2Sender_t* output_chunk) {
    // directly copy the data
    switch (input_chunk->feature_chunk.chunk.type) {
        case NORMAL_CHUNK: {
            output_chunk->send_chunk.header.type = NORMAL_CHUNK;
            output_chunk->send_chunk.header.size = input_chunk->enc_size;
            memcpy(output_chunk->send_chunk.data, input_chunk->enc_data,
                output_chunk->send_chunk.header.size);

            // update the key recipe
            memcpy(output_chunk->key_recipe.key, input_chunk->key,
                CHUNK_HASH_SIZE);
            break;
        }
        case RECIPE_CHUNK: {
            output_chunk->send_chunk.header.type = RECIPE_CHUNK;
            output_chunk->send_chunk.header.size = sizeof(FileRecipeHead_t);
            memcpy(output_chunk->send_chunk.data, &input_chunk->feature_chunk.chunk.head,
                sizeof(FileRecipeHead_t));
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
            exit(EXIT_FAILURE);
        }
    }
    return ;
}

/**
 * @brief enc + comp mode
 * 
 * @param input_chunk input chunk
 * @param output_chunk output chunk
 */
void SelectCompThd::EncCompMode(EncFeatureChunk_t* input_chunk,
    SelectComp2Sender_t* output_chunk) {
    switch (input_chunk->feature_chunk.chunk.type) {
        case NORMAL_CHUNK: {
            output_chunk->send_chunk.header.type = COMPRESSED_NORMAL_CHUNK;
            output_chunk->send_chunk.header.size = comp_pad_->CompressWithPad(
                input_chunk->feature_chunk.chunk.raw_chunk.data,
                input_chunk->feature_chunk.chunk.raw_chunk.size,
                output_chunk->send_chunk.data, input_chunk->seed);

            // update the key recipe
            memcpy(output_chunk->key_recipe.key, input_chunk->key,
                CHUNK_HASH_SIZE);
            break;
        }
        case RECIPE_CHUNK: {
            output_chunk->send_chunk.header.type = RECIPE_CHUNK;
            output_chunk->send_chunk.header.size = sizeof(FileRecipeHead_t);
            memcpy(output_chunk->send_chunk.data, &input_chunk->feature_chunk.chunk.head,
                sizeof(FileRecipeHead_t));
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
            exit(EXIT_FAILURE);
        }
    }
    return ;
}

/**
 * @brief full EDR mode
 * 
 * @param input_chunk input chunk
 * @param cache_chunk cached chunk <ret first if necessary>
 * @param output_chunk output chunk
 * @return true need cached chunk
 * @return false not need cached chunk
 */
bool SelectCompThd::FullEDR(EncFeatureChunk_t* input_chunk,
    SelectComp2Sender_t* cache_chunk, SelectComp2Sender_t* output_chunk) {
    // perform selective local compression
    bool ret = false;
    switch (input_chunk->feature_chunk.chunk.type) {
        case NORMAL_CHUNK: {
            // check the cache meta data
            bool is_similar = false;

#ifdef EDR_BREAKDOWN
            gettimeofday(&_cache_manage_stime, NULL);
#endif

            is_similar = cache_meta_->QueryCacheMeta(input_chunk->feature_chunk.features);

#ifdef EDR_BREAKDOWN
            gettimeofday(&_cache_manage_etime, NULL);
            _total_cache_manage_time += tool::GetTimeDiff(_cache_manage_stime,
                _cache_manage_etime);
            _total_cache_manage_size += input_chunk->enc_size;
#endif

            if (is_similar) {
                // is a similar chunk
                output_chunk->send_chunk.header.type = FULL_EDR_UNCOMPRESS_CHUNK;
                output_chunk->send_chunk.header.size = input_chunk->enc_size;
                memcpy(output_chunk->send_chunk.data, input_chunk->enc_data,
                    output_chunk->send_chunk.header.size);
                
                // copy the cipher feature without re-computing them in the server side
                memcpy(output_chunk->send_chunk.header.cipher_features,
                    input_chunk->feature_chunk.features,
                    sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);

                // perform local compression and generate compressed
                uint8_t compressed_data[ENC_MAX_CHUNK_SIZE];
                uint8_t enc_data[ENC_MAX_CHUNK_SIZE];
                uint64_t compressed_size = comp_pad_->CompressWithPad(
                    input_chunk->feature_chunk.chunk.raw_chunk.data,
                    input_chunk->feature_chunk.chunk.raw_chunk.size,
                    compressed_data,
                    input_chunk->seed);
                
                // encrypt the chunk here
                uint64_t enc_size = two_phase_enc_->TwoPhaseEncChunk(
                    compressed_data,
                    compressed_size,
                    input_chunk->key,
                    enc_data
                );

                crypto_util_->GenerateHash(md_ctx, enc_data, enc_size, 
                    output_chunk->send_chunk.header.compressed_fp);

                // update the key recipe
                memcpy(output_chunk->key_recipe.key, input_chunk->key,
                    CHUNK_HASH_SIZE);
                ret = false;
            } else {
                // prepare the cached chunk (only for caching)
                cache_chunk->send_chunk.header.type = FULL_EDR_CACHE_CHUNK;
                cache_chunk->send_chunk.header.size = input_chunk->enc_size;
                memcpy(cache_chunk->send_chunk.data, input_chunk->enc_data,
                    cache_chunk->send_chunk.header.size);
                
                // copy the cipher feature of the cache chunk
                memcpy(cache_chunk->send_chunk.header.cipher_features,
                    input_chunk->feature_chunk.features,
                    sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);  

#ifdef EDR_BREAKDOWN
                gettimeofday(&_comp_pad_stime, NULL);
#endif

                // perform local compression    
                output_chunk->send_chunk.header.type = COMPRESSED_NORMAL_CHUNK;
                output_chunk->send_chunk.header.size = comp_pad_->CompressWithPad(
                    input_chunk->feature_chunk.chunk.raw_chunk.data,
                    input_chunk->feature_chunk.chunk.raw_chunk.size,
                    output_chunk->send_chunk.data,
                    input_chunk->seed);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_comp_pad_etime, NULL);
                _total_comp_pad_time += tool::GetTimeDiff(_comp_pad_stime,
                    _comp_pad_etime);
                _total_comp_pad_size += input_chunk->feature_chunk.chunk.raw_chunk.size;
#endif

                // update the key recipe
                memcpy(output_chunk->key_recipe.key, input_chunk->key,
                    CHUNK_HASH_SIZE);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cache_manage_stime, NULL);
#endif

                cache_meta_->UpdateCacheMeta(input_chunk->feature_chunk.features);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cache_manage_etime, NULL);
                _total_cache_manage_time += tool::GetTimeDiff(_cache_manage_stime,
                    _cache_manage_etime);
#endif

                ret = true;
            }
            break;
        }
        case RECIPE_CHUNK: {
            output_chunk->send_chunk.header.type = RECIPE_CHUNK;
            output_chunk->send_chunk.header.size = sizeof(FileRecipeHead_t);
            memcpy(output_chunk->send_chunk.data, &input_chunk->feature_chunk.chunk.head,
                sizeof(FileRecipeHead_t));
            
            ret = false;
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
            exit(EXIT_FAILURE);
        }
    }

    return ret;
}