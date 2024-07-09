/**
 * @file abs_mq.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of AbsMQ
 * @version 0.1
 * @date 2022-06-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_ABS_MQ_H
#define MY_CODEBASE_ABS_MQ_H

#include <boost/atomic.hpp>

template <class T>
class AbsMQ {
    protected:

    public:
        // to show whether the whole process is done
        boost::atomic<bool> _done;

        /**
         * @brief Construct a new AbsMQ object
         * 
         */
        AbsMQ() {
            _done = false;
        }

        /**
         * @brief Destroy the AbsMQ object
         * 
         */
        virtual ~AbsMQ() {
            ;
        }

        /**
         * @brief push a data item into the queue
         * 
         * @param data the data item
         * @return true success
         * @return false fails
         */
        virtual bool Push(T& data) = 0;

        /**
         * @brief pop a data item from the queue
         * 
         * @param data the data item
         * @return true success
         * @return false fails
         */
        virtual bool Pop(T& data) = 0;

        /**
         * @brief check whether the queue is empty 
         * 
         * @return true empty
         * @return false not empty
         */
        virtual bool IsEmpty() = 0;
};

#endif