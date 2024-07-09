/**
 * @file leveldbDatabase.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in leveldb database
 * @version 0.1
 * @date 2020-01-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/database/leveldb_db.h"

/**
 * @brief Construct a new Database object
 * 
 * @param db_name the path of the db file
 */
LeveldbDatabase::LeveldbDatabase(string db_name) {
    this->OpenDB(db_name);
}

/**
 * @brief Destroy the Database:: Database object
 * 
 */
LeveldbDatabase::~LeveldbDatabase() {
    string name = "." + db_name_ + ".lock";
    remove(name.c_str());
    delete level_db_obj_;
    delete options_.block_cache;
}

/**
 * @brief open a database
 * 
 * @param db_name the db path 
 * @return true success
 * @return false fail
 */
bool LeveldbDatabase::OpenDB(string db_name) {
    // check whether there exists a lock for the given database
    fstream db_lock;
    db_lock.open("." + db_name + ".lock", std::fstream::in);
    if (db_lock.is_open()) {
        db_lock.close();
        tool::Logging(my_name_.c_str(), "database lock.\n");
        return false;
    }
    db_name_ = db_name;

    options_.create_if_missing = true;
    options_.block_cache = leveldb::NewLRUCache(2 << 30); // 256MB cache
    leveldb::Status status = leveldb::DB::Open(options_, db_name_,
        &level_db_obj_);
    assert(status.ok());
    if (status.ok()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief execute query over database
 * 
 * @param key key
 * @param value value
 * @return true exist
 * @return false not exist
 */
bool LeveldbDatabase::Query(const string& key, string& value) {
    leveldb::Status query_stat = level_db_obj_->Get(leveldb::ReadOptions(), key, &value);
    return query_stat.ok();
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true exist
 * @return false not exist
 */
bool LeveldbDatabase::Insert(const string& key, const string& value) {
    leveldb::Status insert_stat = level_db_obj_->Put(leveldb::WriteOptions(), key, value); 
    return insert_stat.ok();
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key the key 
 * @param buf the value buffer
 * @param buf_size the buffer size
 * @return true exist
 * @return false not exist
 */
bool LeveldbDatabase::InsertBuffer(const string& key, const char* buf, size_t buf_size) {
    leveldb::Status insert_stat = level_db_obj_->Put(leveldb::WriteOptions(),
        key, leveldb::Slice(buf, buf_size));
    return insert_stat.ok();
}

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
bool LeveldbDatabase::InsertBothBuffer(const char* key, size_t key_size, const char* buf,
    size_t buf_size) {
    leveldb::Status insert_stat = level_db_obj_->Put(leveldb::WriteOptions(),
        leveldb::Slice(key, key_size), leveldb::Slice(buf, buf_size));
    return insert_stat.ok();
}

/**
 * @brief query the (key, value) pair
 * 
 * @param key the key
 * @param key_size the key size
 * @param value the value
 * @return true exist
 * @return false not exist
 */
bool LeveldbDatabase::QueryBuffer(const char* key, size_t key_size, string& value) {
    leveldb::Status query_stat = level_db_obj_->Get(leveldb::ReadOptions(),
        leveldb::Slice(key, key_size), &value);
    return query_stat.ok();
}

/**
 * @brief delete a given key
 * 
 * @param key key ptr
 * @param key_size key size
 */
void LeveldbDatabase::DeleteBuffer(const char* key, size_t key_size) {
    level_db_obj_->Delete(leveldb::WriteOptions(),
        leveldb::Slice(key, key_size));
    return ;
}

/**
 * @brief delete a given key
 * 
 * @param key key str
 */
void LeveldbDatabase::Delete(const string& key) {
    level_db_obj_->Delete(leveldb::WriteOptions(), key);
    return ;
}