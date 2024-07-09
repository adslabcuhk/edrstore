/**
 * @file data_retriever_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DataRetrieverThd
 * @version 0.1
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/data_retriever_thd.h"

/**
 * @brief Construct a new DataRetrieverThd object
 * 
 * @param server_channel server channel
 * @param server_conn_record connection record
 * @param file_name_hash file name hash
 * @param method_type method type
 */
DataRetrieverThd::DataRetrieverThd(SSLConnection* server_channel,
    pair<int, SSL*> server_conn_record, uint8_t* file_name_hash,
    uint32_t method_type) {
    // for config
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();
    client_id_ = config.GetClientID();

    // for storage server connection
    server_channel_ = server_channel;
    server_conn_record_ = server_conn_record;
    server_ssl_ = server_conn_record.second;

    // recv chunk buffer
    recv_chunk_buf_.send_buf = (uint8_t*) malloc(send_chunk_batch_size_ *
        sizeof(SendChunk_t) + sizeof(NetworkHead_t));
    recv_chunk_buf_.header = (NetworkHead_t*) recv_chunk_buf_.send_buf;
    recv_chunk_buf_.header->client_id = client_id_;
    recv_chunk_buf_.header->size = 0;
    recv_chunk_buf_.header->cur_item_num = 0;
    recv_chunk_buf_.data_buf = recv_chunk_buf_.send_buf + sizeof(NetworkHead_t); 

    // for key recipe
    key_recipe_buf_.buf = (uint8_t*) malloc(send_recipe_batch_size_ * sizeof(KeyRecipe_t));
    key_recipe_buf_.cnt = 0;

    char file_name_hash_buf[CHUNK_HASH_SIZE * 2 + 1];
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(file_name_hash_buf + i * 2, "%02x", file_name_hash[i]);
    }
    string file_name_str;
    file_name_str.assign(file_name_hash_buf, CHUNK_HASH_SIZE * 2);
    string key_recipe_path = config.GetRecipeRootPath() + file_name_str +
        config.GetKeyRecipeSuffix();
    key_recipe_hdl_.open(key_recipe_path, ios_base::in | ios_base::binary);
    if (!key_recipe_hdl_.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init key recipe file: %s\n",
            key_recipe_path.c_str());
        exit(EXIT_FAILURE);
    }

    // for decryption
    mle_util_ = new MLE();
    method_type_ = method_type;

    // for padding
    comp_pad_ = new CompPad();
}

/**
 * @brief Destroy the DataRetrieverThd object
 * 
 */
DataRetrieverThd::~DataRetrieverThd() {
    key_recipe_hdl_.close();
    free(recv_chunk_buf_.send_buf);
    free(key_recipe_buf_.buf);
    delete mle_util_;
    delete comp_pad_;
}

/**
 * @brief the main process
 * 
 * @param output_MQ output MQ
 */
void DataRetrieverThd::Run(AbsMQ<Retriever2Writer_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    uint32_t recv_size = 0; 
    bool job_done_flag = false;
    while (true) {
        // wait the data from the storage server
        if (!server_channel_->ReceiveData(server_ssl_,
            recv_chunk_buf_.send_buf, recv_size)) {
            tool::Logging(my_name_.c_str(), "recv data from storage server error.\n");
            exit(EXIT_FAILURE);
        } else {
            gettimeofday(&proc_stime, NULL);
            switch (recv_chunk_buf_.header->msg_type) {
                case SERVER_RESTORE_CHUNK: {
                    this->ProcessBatchChunks(output_MQ);
                    break;
                }
                case SERVER_RESTORE_RECIPE: {
                    tool::Logging(my_name_.c_str(), "fix: restore recipe.\n");
                    exit(EXIT_FAILURE);
                }
                case SERVER_RESTORE_FINAL: {
                    server_channel_->Finish(server_conn_record_);
                    output_MQ->_done = true;
                    job_done_flag = true;
                    break;    
                }
                default: {
                    tool::Logging(my_name_.c_str(), "download chunk msg type error.\n");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);
        }

        if (job_done_flag) {
            break;
        }
    }

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    if ((_total_recv_chunk_num != recipe_head_.chunk_num) ||
        (_total_recv_data_size != recipe_head_.size)) {
        tool::Logging(my_name_.c_str(), "download file mismatch error: recv "
            "chunk num: %lu, recv data size: %lu, expected chunk num: %lu, expected "
            "data size: %lu\n", _total_recv_chunk_num, _total_recv_data_size,
            recipe_head_.chunk_num, recipe_head_.size);
    }

    tool::Logging(my_name_.c_str(), "thread exits, total proc time: %lf, "
        "total running time: %lf\n", total_proc_time, total_running_time); 

    return ;
}

/**
 * @brief download login with the file name hash
 * 
 * @param file_name_hash the file name hash
 */
void DataRetrieverThd::DownloadLogin(uint8_t* file_name_hash) {
    SendMsgBuffer_t login_buf;
    login_buf.send_buf = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        CHUNK_HASH_SIZE);
    login_buf.header = (NetworkHead_t*) login_buf.send_buf;
    login_buf.header->client_id = client_id_;
    login_buf.header->size = 0;
    login_buf.data_buf = login_buf.send_buf + sizeof(NetworkHead_t);
    login_buf.header->msg_type = CLIENT_LOGIN_DOWNLOAD;

    memcpy(login_buf.data_buf + login_buf.header->size, file_name_hash,
        CHUNK_HASH_SIZE);
    login_buf.header->size += CHUNK_HASH_SIZE;

    // send the download login request
    if (!server_channel_->SendData(server_ssl_, login_buf.send_buf,
        sizeof(NetworkHead_t) + login_buf.header->size)) {
        tool::Logging(my_name_.c_str(), "send download login error.n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response
    uint32_t recv_size = 0;
    if (!server_channel_->ReceiveData(server_ssl_, login_buf.send_buf,
        recv_size)) {
        tool::Logging(my_name_.c_str(), "recv login response error.\n");
        exit(EXIT_FAILURE);
    }

    switch (login_buf.header->msg_type) {
        case SERVER_FILE_NON_EXIST: {
            tool::Logging(my_name_.c_str(), "the request file not exist.\n");
            exit(EXIT_FAILURE);
        }
        case SERVER_LOGIN_RESPONSE: {
            tool::Logging(my_name_.c_str(), "recv the server login response well, "
                "the server is ready to process the request.\n");
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "server response is wrong, "
                "it is not ready.\n");
            exit(EXIT_FAILURE);
        }
    }

    memcpy(&recipe_head_, login_buf.data_buf, sizeof(FileRecipeHead_t));
    tool::Logging(my_name_.c_str(), "total check num: %lu\n",
        recipe_head_.chunk_num);
    tool::Logging(my_name_.c_str(), "file size: %lu\n",
        recipe_head_.size);

    free(login_buf.send_buf);
    return ;
}

/**
 * @brief fetch key recipe
 * 
 */
void DataRetrieverThd::FetchKeyRecipe() {
    key_recipe_hdl_.read((char*)key_recipe_buf_.buf,
        send_recipe_batch_size_ * sizeof(KeyRecipe_t));
    key_recipe_buf_.cnt = 0;
    remain_recipe_num_ = key_recipe_hdl_.gcount() / sizeof(KeyRecipe_t);
    return ;
}

/**
 * @brief process a batch of chunks
 * 
 * @param output_MQ output MQ
 */
void DataRetrieverThd::ProcessBatchChunks(AbsMQ<Retriever2Writer_t>* output_MQ) {
    uint32_t cur_chunk_num = recv_chunk_buf_.header->cur_item_num;
    SendChunkHeader_t* cur_header;
    uint8_t* cur_data;

    SendChunk_t tmp_chunk;
    size_t offset = 0;
    for (size_t i = 0; i < cur_chunk_num; i++) {
        cur_header = (SendChunkHeader_t*)(recv_chunk_buf_.data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        cur_data = recv_chunk_buf_.data_buf + offset;
        this->ProcessChunk(cur_data, cur_header->size, &tmp_chunk);

        output_MQ->Push(tmp_chunk);
        offset += cur_header->size;

        _total_recv_chunk_num++;
        _total_recv_data_size += tmp_chunk.header.size;
    }

    return ;
}

/**
 * @brief process a chunk
 * 
 * @param input_chunk input chunk
 * @param size chunk size
 * @param output_chunk output chunk
 */
void DataRetrieverThd::ProcessChunk(uint8_t* input_chunk, uint32_t size,
    SendChunk_t* output_chunk) {
    if (remain_recipe_num_ == 0) {
        // fetch a new batch of key recipes
        this->FetchKeyRecipe();
    }

    KeyRecipe_t* tmp_key_recipe = (KeyRecipe_t*) (key_recipe_buf_.buf +
        key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
    switch (method_type_) {
        case PLAIN: {
            // directly copy 
            memcpy(output_chunk->data, input_chunk, size);
            output_chunk->header.size = size;
            break;
        }
        case SERVER_AIDED_MLE: {
            // perform decryption
            mle_util_->DecChunk(input_chunk, size, tmp_key_recipe->key, output_chunk->data);
            output_chunk->header.size = size;
            break;
        }
        case ENC_COMP_MLE: {
            uint8_t tmp_padding_chunk[ENC_MAX_CHUNK_SIZE];
            mle_util_->DecChunk(input_chunk, size, tmp_key_recipe->key, tmp_padding_chunk);

            output_chunk->header.size = comp_pad_->DecompressWithPad(
                tmp_padding_chunk, size, output_chunk->data);

            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong process chunk method.\n");
            exit(EXIT_FAILURE);
        }
    }

    key_recipe_buf_.cnt++;
    remain_recipe_num_--;

    return ;
}