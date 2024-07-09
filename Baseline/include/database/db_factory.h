/**
 * @file db_factory.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the factory of database
 * @version 0.1
 * @date 2020-01-26
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef MY_CODEBASE_DB_FACTORY_H
#define MY_CODEBASE_DB_FACTORY_H

#include "abs_db.h"
#include "in_mem_db.h"
#include "leveldb_db.h"
#include "rocksdb_db.h"

enum DB_TYPE_SET {IN_MEMORY_DB = 0, LEVELDB_DB, ROCKSDB_DB};

class DatabaseFactory {
    private:
        string my_name_ = "DBFactory";
    public:
        AbsDatabase* CreateDatabase(int type, string path);
};

#endif