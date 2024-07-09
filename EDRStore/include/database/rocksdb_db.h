/**
 * @file rocksdb_db.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of using RocksDB
 * @version 0.1
 * @date 2022-08-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_ROCKSDB_DB_H
#define MY_CODEBASE_ROCKSDB_DB_H

#include "abs_db.h"

#include <rocksdb/db.h>
#include <rocksdb/cache.h>
#include <rocksdb/env.h>
#include <rocksdb/table.h>
#include <bits/stdc++.h>

class RocksdbDatabase : public AbsDatabase {
    private:
        string my_name_ = "RocksdbDatabase";
        /* data */
        rocksdb::DB* rocks_db_obj_ = NULL;

        // global setting
        rocksdb::Options options_;
        rocksdb::WriteOptions write_options_;
        rocksdb::ReadOptions read_options_;
    public:
        /**
         * @brief Construct a new RocksdbDatabase object
         * 
         * @param db_name the path of the db file
         */
        RocksdbDatabase(string db_name);

        /**
         * @brief Destroy the RocksdbDatabase object
         * 
         */
        ~RocksdbDatabase();

        /**
         * @brief open a database
         * 
         * @param db_name the db path
         * @return true success
         * @return false fail
         */
        bool OpenDB(string db_name);

        /**
         * @brief query the database
         * 
         * @param key key
         * @param value value
         * @return true exist
         * @return false not exist
         */
        bool Query(const string& key, string& value);

        /**
         * @brief insert the key, value pair
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        bool Insert(const string& key, const string& value);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key the key 
         * @param buf the value buffer
         * @param buf_size the buffer size
         * @return true success
         * @return false fail
         */
        bool InsertBuffer(const string& key, const char* buf, size_t buf_size);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key the key
         * @param key_size the key size
         * @param buf the value buffer
         * @param buf_size the buffer size
         * @return true success
         * @return false fail
         */
        bool InsertBothBuffer(const char* key, size_t key_size, const char* buf,
            size_t buf_size);

        /**
         * @brief query the (key, value) pair
         * 
         * @param key the key
         * @param key_size the key size
         * @param value the value
         * @return true exist
         * @return false not exist
         */
        bool QueryBuffer(const char* key, size_t key_size, string& value);

        /**
         * @brief delete a given key
         * 
         * @param key key ptr
         * @param key_size key size
         */
        void DeleteBuffer(const char* key, size_t key_size);

        /**
         * @brief delete a given key
         * 
         * @param key key str
         */
        void Delete(const string& key);
};

#endif