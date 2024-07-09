/**
 * @file blocked_mq.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of blocked MQ
 * @version 0.1
 * @date 2022-06-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_BLOCKED_MQ_H
#define MY_CODEBASE_BLOCKED_MQ_H

#include "../define.h"
#include "abs_mq.h"

using namespace std;

template <class T>
class BlockedMQ : public AbsMQ<T> {
    private:
        string my_name_ = "BlockedMQ";
        mutex mq_mtx_;
        condition_variable full_sign_;
        condition_variable empty_sign_;
        queue<T> blocked_mq_;

        uint32_t max_queue_size_;

    public:
        /**
         * @brief Construct a new Blocked MQ object
         * 
         * @param max_queue_size the max queue size
         */
        BlockedMQ(uint32_t max_queue_size) {
            max_queue_size_ = max_queue_size;
        }

        /**
         * @brief Destroy the BlockedMQ object
         * 
         */
        ~BlockedMQ() {
            bool flag = this->IsEmpty();
            if (flag) {
                queue<T> empty_mq;
                swap(blocked_mq_, empty_mq);
            } else {
                tool::Logging(my_name_.c_str(), "MQ is not empty.\n");
                exit(EXIT_FAILURE);
            }
        }

        /**
         * @brief push a data item to the queue
         * 
         * @param data the data item
         * @return true success
         * @return false fails
         */
        bool Push(T& data) {
            unique_lock<mutex> lck(mq_mtx_);
            while (blocked_mq_.size() == max_queue_size_) {
                full_sign_.wait(lck);
            }
            assert(blocked_mq_.size() < max_queue_size_);
            blocked_mq_.push(data);
            empty_sign_.notify_all();
            return true;
        }

        /**
         * @brief pop data from the queue
         * 
         * @param data the original data
         * @return true success
         * @return false fails
         */
        bool Pop(T& data) {
            unique_lock<mutex> lck(mq_mtx_);
            while (blocked_mq_.empty()) {
                if (!this->_done) {
                    empty_sign_.wait(lck);
                } else {
                    return false;
                }
            }
            assert(!blocked_mq_.empty());
            data = std::move(blocked_mq_.front()); 
            blocked_mq_.pop();
            full_sign_.notify_all();
            return true;
        }

        /**
         * @brief check whether the queue is empty
         * 
         * @return true success
         * @return false fails
         */
        bool IsEmpty() {
            unique_lock<mutex> lck(mq_mtx_);
            return blocked_mq_.empty();
        }
};

#endif