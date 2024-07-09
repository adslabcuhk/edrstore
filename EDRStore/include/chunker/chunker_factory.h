/**
 * @file chunker_factory.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the factory of chunker
 * @version 0.1
 * @date 2022-06-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MY_CODEBASE_CHUNKER_FACTORY_H
#define MY_CODEBASE_CHUNKER_FACTORY_H

#include "abs_chunker.h"
#include "fix_chunker.h"
#include "fastcdc_chunker.h"

// the type of chunker
enum CHUNKER_TYPE {FIXED_SIZE_CHUNKING = 0, FAST_CDC,
    FSL_TRACE, MS_TRACE};

class ChunkerFactory {
    private:
        string my_name_ = "ChunkerFactory";
    public:
        ChunkerFactory() {
            ;
        }

        ~ChunkerFactory() {
            ;
        }

        AbsChunker* CreateChunker(int type) {
            switch (type) {
                case FAST_CDC: {
                    tool::Logging(my_name_.c_str(), "using FastCDC chunking.\n");
                    return new FastCDC();
                }
                case FIXED_SIZE_CHUNKING: {
                    tool::Logging(my_name_.c_str(), "using Fixed-size chunking.\n");
                    return new FixChunker();
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunker type.\n");
                    exit(EXIT_FAILURE);    
                }
            }
            return NULL;
        }
};

#endif