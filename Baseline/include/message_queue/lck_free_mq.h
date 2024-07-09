/**
 * @file lck_free_mq.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2019-12-31
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef MY_CODEBASE_LCK_FREE_MQ_H
#define MY_CODEBASE_LCK_FREE_MQ_H

#include "../define.h"
#include "abs_mq.h"
#include "readerwriterqueue.h"

using namespace std;

template <class T>
class LckFreeMQ : public AbsMQ<T> {
    private:
        string my_name_ = "LckFreeMQ";
        moodycamel::ReaderWriterQueue<T>* lockFreeQueue_;
        
        uint32_t max_queue_size_;

    public:
        /**
         * @brief Construct a new Message Queue object
         * 
         */
        LckFreeMQ(uint32_t max_queue_size) {
            max_queue_size_ = max_queue_size;
            lockFreeQueue_ = new moodycamel::ReaderWriterQueue<T>(max_queue_size_);
        }

        /**
         * @brief Destroy the LckFreeMQ object
         * 
         */
        ~LckFreeMQ() {
            bool flag = this->IsEmpty();
            if (flag) {
                delete lockFreeQueue_;
            } else {
                tool::Logging(my_name_.c_str(), "MQ is not empty.\n");
                exit(EXIT_FAILURE);
            }
        }

        /**
         * @brief push data to the queue
         * 
         * @param data the original data
         * @return true success
         * @return false fails
         */
        bool Push(T& data) {
            // while (!lockFreeQueue_.push(data)) {
            while (!lockFreeQueue_->try_enqueue(data)) {
                ;
            }
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
            // return lockFreeQueue_.pop(data);
            return lockFreeQueue_->try_dequeue(data);
        }

        /**
         * @brief Decide whether the queue is empty
         * 
         * @return true success
         * @return false fails
         */
        bool IsEmpty() {
            // return lockFreeQueue_.empty();
            size_t count = lockFreeQueue_->size_approx();
            if (count == 0) {
                return true;
            } else {
                return false;
            }
        }
};

#endif // BASICDEDUP_MESSAGEQUEUE_h
