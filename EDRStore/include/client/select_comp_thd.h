/**
 * @file select_comp_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of selective compression thread
 * @version 0.1
 * @date 2022-06-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_SELECT_COMP_THD_H
#define EDRSTORE_SELECT_COMP_THD_H

#include "two_phase_enc.h"
#include "comp_pad.h"
#include "cache_meta.h"
#include "../reduction/similar_policy.h"
#include "../data_structure.h"
#include "../message_queue/mq_factory.h"

class SelectCompThd {
    private:
        string my_name_ = "SelectCompThd";

        // for config
        uint32_t method_type_;

        // compression padding
        CompPad* comp_pad_;

        // two-phase encryption
        TwoPhaseEnc* two_phase_enc_;

        // cache meta
        CacheMeta* cache_meta_;

        // for crypto
        CryptoUtil* crypto_util_;
        EVP_MD_CTX* md_ctx;

        /**
         * @brief only enc mode
         * 
         * @param input_chunk input chunk
         * @param output_chunk output chunk
         */
        void OnlyEncMode(EncFeatureChunk_t* input_chunk, SelectComp2Sender_t* output_chunk);

        /**
         * @brief enc + comp mode
         * 
         * @param input_chunk input chunk
         * @param output_chunk output chunk
         */
        void EncCompMode(EncFeatureChunk_t* input_chunk, SelectComp2Sender_t* output_chunk);

        /**
         * @brief full EDR mode
         * 
         * @param input_chunk input chunk
         * @param cache_chunk cached chunk <ret first if necessary>
         * @param output_chunk output chunk
         * @return true need cached chunk
         * @return false not need cached chunk
         */
        bool FullEDR(EncFeatureChunk_t* input_chunk, SelectComp2Sender_t*
            cache_chunk, SelectComp2Sender_t* output_chunk);

    public:
#ifdef EDR_BREAKDOWN
        struct timeval _comp_pad_stime;
        struct timeval _comp_pad_etime;
        double _total_comp_pad_time = 0;
        uint64_t _total_comp_pad_size = 0;

        struct timeval _cache_manage_stime;
        struct timeval _cache_manage_etime;
        double _total_cache_manage_time = 0;
        uint64_t _total_cache_manage_size = 0;
#endif
        /**
         * @brief Construct a new SelectCompThd object
         * 
         * @param cache_meta cache meta data
         * @param method_type method type 
         */
        SelectCompThd(CacheMeta* cache_meta, uint32_t method_type);

        /**
         * @brief Destroy the Select Comp Thd object
         * 
         */
        ~SelectCompThd();

        /**
         * @brief the main thread
         * 
         * @param input_MQ the input MQ
         */
        void Run(AbsMQ<EncFeatureChunk_t>* input_MQ,
            AbsMQ<SelectComp2Sender_t>* output_MQ);
};

#endif