/**
 * @file select_comp_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of selective compression thread
 * @version 0.1
 * @date 2022-07-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_SELECT_COMP_THD_H
#define EDRSTORE_SELECT_COMP_THD_H

#include "comp_pad.h"
#include "mle.h"
#include "../data_structure.h"
#include "../configure.h"
#include "../message_queue/mq_factory.h"

using namespace std;

extern Configure config;

class SelectCompThd {
    private:
        string my_name_ = "SelectCompThd";

        // for config
        uint32_t method_type_;

        // compression padding
        CompPad* comp_pad_;

        // for encryption
        MLE* mle_util_;

    public:
        /**
         * @brief Construct a new SelectCompThd object
         * 
         * @param method_type method type
         */
        SelectCompThd(uint32_t method_type);

        /**
         * @brief Destroy the SelectCompThd object
         * 
         */
        ~SelectCompThd();

        /**
         * @brief the main thread
         * 
         * @param input_MQ the input MQ
         * @param output_MQ the output MQ
         */
        void Run(AbsMQ<KeyGen2SelectComp_t>* input_MQ,
            AbsMQ<SelectComp2Sender_t>* output_MQ);
};

#endif