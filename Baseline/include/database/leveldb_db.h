/**
 * @file leveldbDatabase.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implementation the database based on leveldb
 * @version 0.1
 * @date 2020-01-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MY_CODEBASE_LEVELDB_DB_H
#define MY_CODEBASE_LEVELDB_DB_H

#include "abs_db.h"

#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <bits/stdc++.h>

class LeveldbDatabase : public AbsDatabase {
    protected:
        string my_name_ = "LeveldbDatabase";
        /* data */
        leveldb::DB* level_db_obj_ = NULL;
        leveldb::Options options_;

    public:
        /**
         * @brief Construct a new Database object
         * 
         * @param db_name the path of the db file
         */
        LeveldbDatabase(string db_name);

        /**
         * @brief Destroy the leveldb Database object
         * 
         */
        ~LeveldbDatabase();

        /**
         * @brief open a database
         * 
         * @param db_name the db path 
         * @return true success
         * @return false fail
         */
        bool OpenDB(string db_name);

        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true exist
         * @return false not exist
         */
        bool Query(const string& key, string& value);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true exist
         * @return false not exist
         */
        bool Insert(const string& key, const string& value);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key the key 
         * @param buf the value buffer
         * @param buf_size the buffer size
         * @return true exist
         * @return false not exist
         */
        bool InsertBuffer(const string& key, const char* buf, size_t buf_size);

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