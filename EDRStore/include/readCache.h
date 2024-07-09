/**
 * @file readCache.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of read container cache 
 * @version 0.1
 * @date 2020-02-07
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef BASICDEDUP_READCACHE_H
#define BASICDEDUP_READCACHE_H

#include "lruCache.h"

using namespace std;

class ReadCache {
    private:
        // <container-ID, index of container pool>
        lru11::Cache<string, uint32_t>* readCache_;

        // container cache space pointer
        uint8_t** containerPool_;

        // container cache size and item size
        uint64_t cacheSize_ = 0;
        uint64_t itemSize_ = 0;

        size_t currentIndex_ = 0;
    public:
        /**
         * @brief Construct a new Read Cache object
         * 
         * @param itemNum the number of the item
         * @param itemSize the size of each item
         */
        ReadCache(size_t itemNum, size_t itemSize);

        /**
         * @brief Destroy the Read Cache object
         * 
         */
        ~ReadCache();

        /**
         * @brief insert the data to the cache
         * 
         * @param name key
         * @param data data
         * @param length the length of the container section
         */
        void Insert(string& name, uint8_t* data, uint32_t length);


        /**
         * @brief check whether this item exists in the cache
         * 
         * @param name 
         * @return true 
         * @return false 
         */
        bool Exist(string& name);


        /**
         * @brief read container from cache
         * 
         * @param name id of the container
         * @return uint8_t* container data
         */
        uint8_t* Read(string& name);

};

#endif // !BASICDEDUP_READCACHE_H




