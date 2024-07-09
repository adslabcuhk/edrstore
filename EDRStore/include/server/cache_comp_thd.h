/**
 * @file cache_comp_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of CacheCompThd
 * @version 0.1
 * @date 2022-08-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_CACHE_COMP_THD
#define EDRSTORE_CACHE_COMP_THD

#include "client_var.h"
#include "../database/db_factory.h"

class CacheCompThd {
    private:
        string my_name_ = "CacheCompThd";

    public:
        // for delta compression
        uint64_t _total_similar_chunk_num = 0;
        uint64_t _total_similar_data_size = 0;
        uint64_t _total_delta_size = 0;

#ifdef EDR_BREAKDOWN
        struct timeval _cache_manage_stime;
        struct timeval _cache_manage_etime;
        double _total_cache_manage_time = 0;
        uint64_t _total_cache_manage_data_size = 0;

        struct timeval _cache_delta_stime;
        struct timeval _cache_delta_etime;
        double _total_cache_delta_time = 0;
        uint64_t _total_cache_delta_data_size = 0;
#endif

        /**
         * @brief Construct a new CacheCompThd object
         * 
         */
        CacheCompThd();

        /**
         * @brief Destroy the CacheCompThd object
         * 
         */
        ~CacheCompThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client var
         */
        void Run(ClientVar* cur_client);
};

#endif