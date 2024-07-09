/**
 * @file cache_comp_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of CacheCompThd
 * @version 0.1
 * @date 2022-08-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/cache_comp_thd.h"

/**
 * @brief Construct a new CacheCompThd object
 * 
 */
CacheCompThd::CacheCompThd() {

}

/**
 * @brief Destroy the CacheCompThd object
 * 
 */
CacheCompThd::~CacheCompThd() {

}

/**
 * @brief the main process
 * 
 * @param cur_client current client var
 */
void CacheCompThd::Run(ClientVar* cur_client) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");
    AbsMQ<WrappedChunk_t>* input_MQ = cur_client->_dual_2_comp_mq;
    AbsMQ<WrappedChunk_t>* output_MQ = cur_client->_comp_2_writer_mq;
    InformCache* inform_cache = cur_client->_inform_cache;

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------
    WrappedChunk_t input_data;
    WrappedChunk_t output_data;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(input_data)) {
            gettimeofday(&proc_stime, NULL);
            switch (input_data.info.stat) {
                case UNIQUE_CHUNK_AFTER_CACHE: {
                    // previous chunk is cached chunk
                    input_data.info.stat = UNIQUE_CHUNK;
                    output_MQ->Push(input_data);
                    break;
                }
                case UNIQUE_CHUNK: {
                    // previous cache is not a insert cache chunk, check cache
#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_delta_stime, NULL);
#endif
                    bool is_similar = inform_cache->ProcessNormalChunk(&input_data,
                        &output_data);

#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_delta_etime, NULL);
                    _total_cache_delta_time += tool::GetTimeDiff(_cache_delta_stime,
                        _cache_delta_etime);
                    _total_cache_delta_data_size += input_data.info.size;
#endif

                    if(is_similar) {
                        // similar chunk
                        output_MQ->Push(output_data);
                        _total_similar_chunk_num++;
                        _total_similar_data_size += input_data.info.size;
                        _total_delta_size += output_data.info.size;
                    } else {
                        // non-similar chunk, back to original stat
                        if (inform_cache->GetCacheSize() == 0) {
                            input_data.info.stat = UNIQUE_CHUNK;
                            output_MQ->Push(input_data);
                        } else {
                            tool::Logging(my_name_.c_str(), "wrong chunk type in cache.\n");
                            exit(EXIT_FAILURE);
                        }
                    }

// #ifdef EDR_BREAKDOWN
//                     gettimeofday(&_cache_delta_etime, NULL);
//                     _total_cache_delta_time += tool::GetTimeDiff(_cache_delta_stime,
//                         _cache_delta_etime);
//                     _total_cache_delta_data_size += input_data.info.size;
// #endif
                    break;
                }
                case CACHE_INSERT_CHUNK: {
#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_manage_stime, NULL);
#endif

                    inform_cache->InsertCachedChunk(&input_data);

#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_manage_etime, NULL);
                    _total_cache_manage_time += tool::GetTimeDiff(_cache_manage_stime,
                        _cache_manage_etime);
                    _total_cache_manage_data_size += input_data.info.size;
#endif
                    break;
                }
                case CACHE_EVICT_CHUNK: {
#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_manage_stime, NULL);
#endif
                    inform_cache->EvictCacheChunk(&input_data);

#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cache_manage_etime, NULL);
                    _total_cache_manage_time += tool::GetTimeDiff(_cache_manage_stime,
                        _cache_manage_etime);
#endif

                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk input type.\n");
                    exit(EXIT_FAILURE);
                }
            }

            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);
        }
    }

    output_MQ->_done = true;
    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total proc time: %lf, "
        "total running time: %lf\n", total_proc_time, total_running_time);

    return ;
}