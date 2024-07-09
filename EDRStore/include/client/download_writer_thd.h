/**
 * @file download_writer_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DownloadWriterThd
 * @version 0.1
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DOWNLOAD_WRITER_THD_H
#define EDRSTORE_DOWNLOAD_WRITER_THD_H

#include "../configure.h"
#include "../message_queue/mq_factory.h"
#include "../data_structure.h"

class DownloadWriterThd {
    private:
        string my_name_ = "DownloadWriterThd";

        // download file
        FILE* download_file_hdl_;

    public:
        uint64_t _total_write_data_size = 0;
        uint64_t _total_write_chunk_num = 0;

        /**
         * @brief Construct a new DownloadWriterThd object
         * 
         * @param file_name download file name
         */
        DownloadWriterThd(string file_name);

        /**
         * @brief Destroy the DownloadWriterThd object
         * 
         */
        ~DownloadWriterThd();

        /**
         * @brief the main process
         * 
         * @param input_MQ input MQ
         */
        void Run(AbsMQ<SendChunk_t>* input_MQ);
};

#endif