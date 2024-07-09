/**
 * @file sender_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of SenderThd
 * @version 0.1
 * @date 2022-07-03
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/sender_thd.h"

/**
 * @brief Construct a new SenderThd object
 * 
 * @param server_channel the connection to the storage server
 * @param server_conn_record the storage server connection record
 * @param file_name_hash the file name hash
 */
SenderThd::SenderThd(SSLConnection* server_channel, 
    pair<int, SSL*> server_conn_record, uint8_t* file_name_hash) {
    // for config
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();
    client_id_ = config.GetClientID();

    // send chunk buffer
    send_chunk_buf_.send_buf = (uint8_t*) malloc(2 * send_chunk_batch_size_ *
        sizeof(SendChunk_t) + sizeof(NetworkHead_t));
    send_chunk_buf_.header = (NetworkHead_t*) send_chunk_buf_.send_buf;
    send_chunk_buf_.header->client_id = client_id_;
    send_chunk_buf_.header->size = 0;
    send_chunk_buf_.header->cur_item_num = 0;
    send_chunk_buf_.data_buf = send_chunk_buf_.send_buf + sizeof(NetworkHead_t); 

    // for storage server connection
    server_channel_ = server_channel;
    server_conn_record_ = server_conn_record;
    server_ssl_ = server_conn_record.second;

    // for key recipe
    key_recipe_buf_.buf = (uint8_t*) malloc(send_recipe_batch_size_ *
        sizeof(KeyRecipe_t));
    key_recipe_buf_.cnt = 0;

    char file_name_hash_buf[CHUNK_HASH_SIZE * 2 + 1];
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(file_name_hash_buf + i * 2, "%02x", file_name_hash[i]);
    }
    string file_name_str;
    file_name_str.assign(file_name_hash_buf, CHUNK_HASH_SIZE * 2);
    string key_recipe_path = config.GetRecipeRootPath() + file_name_str +
        config.GetKeyRecipeSuffix();
    key_recipe_hdl_.open(key_recipe_path, ios_base::trunc | ios_base::binary);
    if (!key_recipe_hdl_.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init key recipe file: %s\n",
            key_recipe_path.c_str());
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Destroy the SenderThd object
 * 
 */
SenderThd::~SenderThd() {
    key_recipe_hdl_.close();
    free(send_chunk_buf_.send_buf);
    free(key_recipe_buf_.buf);
}

/**
 * @brief the main thread
 * 
 * @param input_MQ input MQ
 */
void SenderThd::Run(AbsMQ<SelectComp2Sender_t>* input_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    SelectComp2Sender_t tmp_data;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data)) {
            switch (tmp_data.send_chunk.header.type) {
                case NORMAL_CHUNK: {
                    // this is a normal chunk:
                    // compressed chunk / uncompressed similar chunk
                    memcpy(send_chunk_buf_.data_buf + send_chunk_buf_.header->size,
                        &tmp_data.send_chunk.header, sizeof(SendChunkHeader_t));
                    send_chunk_buf_.header->size += sizeof(SendChunkHeader_t);
                    memcpy(send_chunk_buf_.data_buf + send_chunk_buf_.header->size,
                        tmp_data.send_chunk.data, tmp_data.send_chunk.header.size);
                    send_chunk_buf_.header->size += tmp_data.send_chunk.header.size;

                    send_chunk_buf_.header->cur_item_num ++;
                    
                    if(send_chunk_buf_.header->cur_item_num %
                        send_chunk_batch_size_ == 0){
                        this->SendChunks();
                    }

                    // store the key recipe
                    this->StoreKeyRecipe(&tmp_data.key_recipe);
                    break;
                }
                case RECIPE_CHUNK: {
                    // this is the end recipe chunk
                    if (send_chunk_buf_.header->cur_item_num != 0) {
                        this->SendChunks();
                    }
                    if (key_recipe_buf_.cnt != 0) {
                        key_recipe_hdl_.write((char*)key_recipe_buf_.buf,
                            key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
                        key_recipe_buf_.cnt = 0;
                    }
                    this->ProcessRecipeEnd(&tmp_data.send_chunk);
                    server_channel_->Finish(server_conn_record_);
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total running time: %lf\n",
        total_running_time);
    
    return ;
}

/**
 * @brief send a batch of chunks
 * 
 */
void SenderThd::SendChunks() {
    send_chunk_buf_.header->msg_type = CLIENT_UPLOAD_CHUNK;
    if (!server_channel_->SendData(server_ssl_,
        send_chunk_buf_.send_buf,
        send_chunk_buf_.header->size + sizeof(NetworkHead_t))) {
        tool::Logging(my_name_.c_str(), "send the chunk batch error.\n");
        exit(EXIT_FAILURE);
    }

    // clear the current send chunk buffer
    send_chunk_buf_.header->cur_item_num = 0;
    send_chunk_buf_.header->size = 0;

    return ;
}

/**
 * @brief process the end of the recipe
 * 
 * @param input_chunk the input recipe data
 */
void SenderThd::ProcessRecipeEnd(SendChunk_t* input_chunk) {
    send_chunk_buf_.header->msg_type = CLIENT_UPLOAD_RECIPE_END;
    // copy the recipe end to the send buffer
    memcpy(send_chunk_buf_.data_buf + send_chunk_buf_.header->size,
        input_chunk->data, input_chunk->header.size);
    send_chunk_buf_.header->size += input_chunk->header.size;

    if (!server_channel_->SendData(server_ssl_,
        send_chunk_buf_.send_buf,
        send_chunk_buf_.header->size + sizeof(NetworkHead_t))) {
        tool::Logging(my_name_.c_str(), "send the recipe end error.\n");
        exit(EXIT_FAILURE);
    }

    return ;
}

/**
 * @brief store the key recipe
 * 
 * @param key_recipe the ptr to the key recipe
 */
void SenderThd::StoreKeyRecipe(KeyRecipe_t* key_recipe) {
    // copy the key to the buffer
    memcpy(key_recipe_buf_.buf + key_recipe_buf_.cnt * sizeof(KeyRecipe_t),
        key_recipe, sizeof(KeyRecipe_t));
    key_recipe_buf_.cnt++;

    if (key_recipe_buf_.cnt % send_recipe_batch_size_ == 0) {
        key_recipe_hdl_.write((char*)key_recipe_buf_.buf,
            key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
        key_recipe_buf_.cnt = 0;
    }

    return ;
}

/**
 * @brief upload login with the file name hash
 * 
 * @param file_name_hash the file name hash
 */
void SenderThd::UploadLogin(uint8_t* file_name_hash) {
    SendMsgBuffer_t login_buf;
    login_buf.send_buf = (uint8_t*) malloc(sizeof(NetworkHead_t) +
        CHUNK_HASH_SIZE);
    login_buf.header = (NetworkHead_t*) login_buf.send_buf;
    login_buf.header->client_id = client_id_;
    login_buf.header->size = 0;
    login_buf.data_buf = login_buf.send_buf + sizeof(NetworkHead_t);
    login_buf.header->msg_type = CLIENT_LOGIN_UPLOAD;

    memcpy(login_buf.data_buf + login_buf.header->size, file_name_hash,
        CHUNK_HASH_SIZE);
    login_buf.header->size += CHUNK_HASH_SIZE;

    // send the upload login request
    if (!server_channel_->SendData(server_ssl_, login_buf.send_buf,
        sizeof(NetworkHead_t) + login_buf.header->size)) {
        tool::Logging(my_name_.c_str(), "send upload login error.\n");
        exit(EXIT_FAILURE);
    }

    // wait the server to send the login response
    uint32_t recv_size = 0;
    if (!server_channel_->ReceiveData(server_ssl_, login_buf.send_buf,
        recv_size)) {
        tool::Logging(my_name_.c_str(), "recv login response error.\n");
        exit(EXIT_FAILURE);
    }

    if (login_buf.header->msg_type == SERVER_LOGIN_RESPONSE) {
        tool::Logging(my_name_.c_str(), "server can process the request.\n");
    } else {
        tool::Logging(my_name_.c_str(), "server response is wrong (not ready).\n");
        exit(EXIT_FAILURE);
    }

    free(login_buf.send_buf);
    return ;
}