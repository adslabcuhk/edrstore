/**
 * @file db_factory.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface defined in database factory
 * @version 0.1
 * @date 2020-01-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/database/db_factory.h"

AbsDatabase* DatabaseFactory::CreateDatabase(int type, string path) {
    switch (type) {
        case LEVELDB_DB: {
            tool::Logging(my_name_.c_str(), "using LevelDB.\n");
            return new LeveldbDatabase(path);
        }
        case IN_MEMORY_DB: {
            tool::Logging(my_name_.c_str(), "using In-Memory DB.\n");
            return new InMemoryDatabase(path);
        }
        case ROCKSDB_DB: {
            tool::Logging(my_name_.c_str(), "using RocksDB.\n");
            return new RocksdbDatabase(path);
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong DB type.\n");
            exit(EXIT_FAILURE);    
        }
    }
    return NULL;
}