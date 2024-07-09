/**
 * @file rocksdb_db.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of RocksdbDatabase
 * @version 0.1
 * @date 2022-08-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/database/rocksdb_db.h"

/**
 * @brief Construct a new RocksdbDatabase object
 * 
 * @param db_name the path of the db file
 */
RocksdbDatabase::RocksdbDatabase(string db_name) {
    db_name_ = db_name;
    this->OpenDB(db_name);
}

/**
 * @brief Destroy the RocksdbDatabase object
 * 
 */
RocksdbDatabase::~RocksdbDatabase() {
    delete rocks_db_obj_;
}

/**
 * @brief open a database
 * 
 * @param db_name the db path
 * @return true success
 * @return false fail
 */
bool RocksdbDatabase::OpenDB(string db_name) {
    // global option
    options_.create_if_missing = true;
    options_.IncreaseParallelism(16);
    options_.OptimizeLevelStyleCompaction();
    options_.OptimizeForPointLookup(4096);
    options_.db_write_buffer_size = static_cast<size_t>(128) << 20;
    options_.max_write_buffer_number = 5;
    options_.min_write_buffer_number_to_merge = 3;
    options_.info_log_level = static_cast<rocksdb::InfoLogLevel>(4); // FATAL_LEVEL
    options_.compression = rocksdb::CompressionType::kNoCompression;

    // write option
    write_options_ = rocksdb::WriteOptions();
    write_options_.disableWAL = true;

    // read option
    read_options_ = rocksdb::ReadOptions();

    rocksdb::Status status = rocksdb::DB::Open(options_, db_name_,
        &this->rocks_db_obj_);
    assert(status.ok());
    return true;
}

/**
 * @brief execute query over database
 * 
 * @param key key
 * @param value value
 * @return true exist
 * @return false not exist
 */
bool RocksdbDatabase::Query(const string& key, string& value) {
    rocksdb::Status query_stat = rocks_db_obj_->Get(read_options_, key, &value);
    return query_stat.ok();
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool RocksdbDatabase::Insert(const string& key, const string& value) {
    rocksdb::Status insert_stat = rocks_db_obj_->Put(write_options_, key, value); 
    return insert_stat.ok();
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key the key 
 * @param buf the value buffer
 * @param buf_size the buffer size
 * @return true success
 * @return false fail
 */
bool RocksdbDatabase::InsertBuffer(const string& key, const char* buf, size_t buf_size) {
    rocksdb::Status insert_stat = rocks_db_obj_->Put(write_options_,
        key, rocksdb::Slice(buf, buf_size));
    return insert_stat.ok();
}

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
bool RocksdbDatabase::InsertBothBuffer(const char* key, size_t key_size, const char* buf,
    size_t buf_size) {
    rocksdb::Status insert_stat = rocks_db_obj_->Put(write_options_,
        rocksdb::Slice(key, key_size), rocksdb::Slice(buf, buf_size));
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
bool RocksdbDatabase::QueryBuffer(const char* key, size_t key_size, string& value) {
    rocksdb::Status query_stat = rocks_db_obj_->Get(read_options_,
        rocksdb::Slice(key, key_size), &value);
    return query_stat.ok();
}

/**
 * @brief delete a given key
 * 
 * @param key key ptr
 * @param key_size key size
 */
void RocksdbDatabase::DeleteBuffer(const char* key, size_t key_size) {
    rocks_db_obj_->Delete(write_options_, rocksdb::Slice(key, key_size));
    return ;
}

/**
 * @brief delete a given key
 * 
 * @param key key str
 */
void RocksdbDatabase::Delete(const string& key) {
    rocks_db_obj_->Delete(write_options_, key);
    return ;
}