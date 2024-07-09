/**
 * @file in_mem_db.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of in-memory index
 * @version 0.1
 * @date 2021-05-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/database/in_mem_db.h"

/**
 * @brief Construct a new In Memory Database object
 * 
 * @param db_name the path of the db file
 */
InMemoryDatabase::InMemoryDatabase(string db_name) {
    this->OpenDB(db_name);
    pthread_rwlock_init(&rwlock_, NULL);
}

/**
 * @brief Destroy the In Memory Database object
 * 
 */
InMemoryDatabase::~InMemoryDatabase() {
    // persistent the indexFile to the disk
    ofstream db_file;
    db_file.open(db_name_, ios_base::trunc | ios_base::binary);
    uint32_t item_size = 0;
    for (auto it = index_obj_.begin(); it != index_obj_.end(); it++) {
        // write the key
        item_size = it->first.size();
        db_file.write((char*)&item_size, sizeof(uint32_t));
        db_file.write(it->first.c_str(), item_size);

        // write the value
        item_size = it->second.size();
        db_file.write((char*)&item_size, sizeof(uint32_t));
        db_file.write(it->second.c_str(), item_size);
    }
    db_file.close();
    pthread_rwlock_destroy(&rwlock_);
}

/**
 * @brief open a database
 * 
 * @param db_name the db path 
 * @return true success
 * @return false fail
 */
bool InMemoryDatabase::OpenDB(string db_name) {
    db_name_ = db_name;
    // check whether there exists the index
    ifstream db_file;
    db_file.open(db_name_, ios_base::in | ios_base::binary);
    if (!db_file.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot open the db file.\n");
    }

    size_t start_size = db_file.tellg();
    db_file.seekg(0, ios_base::end);
    size_t fileSize = db_file.tellg();
    fileSize = fileSize - start_size;

    if (fileSize == 0) {
        // db file not exist
        tool::Logging(my_name_.c_str(), "db file file not exist, create a new one.\n");
    } else {
        // db file exist, load
        db_file.seekg(0, ios_base::beg);
        bool is_end = false;
        uint32_t item_size = 0;
        string key;
        string value;
        while (!is_end) {
            // read key
            db_file.read((char*)&item_size, sizeof(uint32_t));
            if (item_size == 0) {
                break;
            }
            key.resize(item_size, 0); 
            db_file.read((char*)&key[0], item_size);

            // read value
            db_file.read((char*)&item_size, sizeof(uint32_t));
            value.resize(item_size, 0);
            db_file.read((char*)&value[0], item_size);
            is_end = db_file.eof();
            
            // update the index
            index_obj_[key] = value;
            item_size = 0;

            if (is_end) {
                break;
            }
        }
    }
    db_file.close();
    tool::Logging(my_name_.c_str(), "loaded index size: %lu\n",
        index_obj_.size());
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
bool InMemoryDatabase::Query(const string& key, string& value) {
    pthread_rwlock_rdlock(&rwlock_);
    bool ret;
    auto find_ret = index_obj_.find(key);
    if (find_ret != index_obj_.end()) {
        // it exists in the index
        value.assign(find_ret->second);
        ret = true;
    } else {
        // it does not exist
        ret = false;
    }
    pthread_rwlock_unlock(&rwlock_);
    return ret;
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true exist
 * @return false not exist
 */
bool InMemoryDatabase::Insert(const string& key, const string& value) {
    pthread_rwlock_wrlock(&rwlock_);
    index_obj_[key] = value;
    pthread_rwlock_unlock(&rwlock_);
    return true;
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
bool InMemoryDatabase::InsertBuffer(const string& key, const char* buf, size_t buf_size) {
    pthread_rwlock_wrlock(&rwlock_);
    string value_str;
    value_str.assign(buf, buf_size);
    index_obj_[key] = value_str;
    pthread_rwlock_unlock(&rwlock_);
    return true;
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
bool InMemoryDatabase::InsertBothBuffer(const char* key, size_t key_size, const char* buf,
    size_t buf_size) {
    pthread_rwlock_wrlock(&rwlock_);
    string key_str;
    string value_str;
    key_str.assign(key, key_size);
    value_str.assign(buf, buf_size);
    index_obj_[key_str] = value_str;
    pthread_rwlock_unlock(&rwlock_);
    return true;
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
bool InMemoryDatabase::QueryBuffer(const char* key, size_t key_size, string& value) {
    pthread_rwlock_rdlock(&rwlock_);
    string key_str;
    bool ret;
    key_str.assign(key, key_size);
    auto find_ret = index_obj_.find(key_str);
    if (find_ret != index_obj_.end()) {
        // it exists in the index
        value.assign(find_ret->second);
        ret = true;
    } else {
        // it does not exist
        ret = false;
    }
    pthread_rwlock_unlock(&rwlock_);
    return ret;
}

/**
 * @brief delete a given key
 * 
 * @param key key ptr
 * @param key_size key size
 */
void InMemoryDatabase::DeleteBuffer(const char* key, size_t key_size) {
    string key_str;
    key_str.assign(key, key_size);
    index_obj_.erase(key_str);
    return ;
}

/**
 * @brief delete a given key
 * 
 * @param key key str
 */
void InMemoryDatabase::Delete(const string& key) {
    index_obj_.erase(key);
    return ;
}