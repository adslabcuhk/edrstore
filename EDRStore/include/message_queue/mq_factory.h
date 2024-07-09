/**
 * @file mq_factory.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the factory of MQ
 * @version 0.1
 * @date 2022-06-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_MQ_FACTORY_H
#define MY_CODEBASE_MQ_FACTORY_H

#include "abs_mq.h"
#include "blocked_mq.h"
#include "lck_free_mq.h"

enum MQ_TYPE_SET {BLOCKED_MQ = 0, LCK_FREE_MQ};

template <class T>
class MQFactory {
    private:
        string my_name_ = "MQFactory";
        
    public:
        /**
         * @brief create a MQ
         * 
         * @param type the MQ type
         * @return AbsMQ* a MQ 
         */
        AbsMQ<T>* CreateMQ(int type, uint32_t max_queue_size) {
            switch (type) {
                case BLOCKED_MQ: {
                    tool::Logging(my_name_.c_str(), "using BlockedMQ.\n");
                    return new BlockedMQ<T>(max_queue_size);
                }
                case LCK_FREE_MQ: {
                    tool::Logging(my_name_.c_str(), "using LckFreeMQ.\n");
                    return new LckFreeMQ<T>(max_queue_size);
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong MQ type.\n");
                    exit(EXIT_FAILURE);
                }
            }
            return NULL;
        }
};

#endif