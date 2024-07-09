/**
 * @file download_writer_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DownloadWriterThd
 * @version 0.1
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/download_writer_thd.h"

/**
 * @brief Construct a new DownloadWriterThd object
 * 
 * @param file_name download file name
 */
DownloadWriterThd::DownloadWriterThd(string file_name) {
    string real_file_name = file_name + "-d";
    download_file_hdl_ = fopen(real_file_name.c_str(), "wb");
}

DownloadWriterThd::~DownloadWriterThd() {
    fprintf(stderr, "========DownloadWriterThd Info========\n");
    fprintf(stderr, "write chunk num: %lu\n", _total_write_chunk_num);
    fprintf(stderr, "write data size: %lu\n", _total_write_data_size);
    fprintf(stderr, "======================================\n");
}

/**
 * @brief the main process
 * 
 * @param input_MQ input MQ
 */
void DownloadWriterThd::Run(AbsMQ<SendChunk_t>* input_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    SendChunk_t tmp_data;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data)) {
            // write the data to the file
            fwrite((char*)tmp_data.data, tmp_data.header.size, 1,
                download_file_hdl_);
            _total_write_chunk_num++;
            _total_write_data_size += tmp_data.header.size; 
        }
    }

    // ensure all data is written to the disk
    fsync(fileno(download_file_hdl_));
    fclose(download_file_hdl_);

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total running time: %lf\n",
        total_running_time);

    return ;
}