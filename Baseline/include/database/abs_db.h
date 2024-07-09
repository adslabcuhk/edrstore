/**
 * @file abs_db.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2020-01-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef MY_CODEBASE_ABS_DB_H
#define MY_CODEBASE_ABS_DB_H

#include "../define.h"

using namespace std;

class AbsDatabase {
    protected:
        string db_name_;

    public:
        /**
         * @brief Construct a new Abs Database object
         * 
         */
        AbsDatabase(); 

        /**
         * @brief Destroy the Abs Database object
         * 
         */
        virtual ~AbsDatabase() {}; 

        /**
         * @brief open a database
         * 
         * @param db_name the db path 
         * @return true success
         * @return false fail
         */
        virtual bool OpenDB(string db_name) = 0;

        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true exist
         * @return false not exist
         */
        virtual bool Query(const string& key, string& value) = 0;

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true exist
         * @return false not exist
         */
        virtual bool Insert(const string& key, const string& value) = 0;

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key the key 
         * @param buf the value buffer
         * @param buf_size the buffer size
         * @return true exist
         * @return false not exist
         */
        virtual bool InsertBuffer(const string& key, const char* buf, size_t buf_size) = 0;

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key the key
         * @param key_size the key size
         * @param buf the value buffer
         * @param buf_size the buffer size
         * @return true exist
         * @return false not exist
         */
        virtual bool InsertBothBuffer(const char* key, size_t key_size, const char* buf,
            size_t buf_size) = 0;

        /**
         * @brief query the (key, value) pair
         * 
         * @param key the key
         * @param key_size the key size
         * @param value the value
         * @return true exist
         * @return false not exist
         */
        virtual bool QueryBuffer(const char* key, size_t key_size, string& value) = 0;

        /**
         * @brief delete a given key
         * 
         * @param key key ptr
         * @param key_size key size
         */
        virtual void DeleteBuffer(const char* key, size_t key_size) = 0;

        /**
         * @brief delete a given key
         * 
         * @param key key str
         */
        virtual void Delete(const string& key) = 0;
};

#endif