/**
 * @file data_receiver.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DataRecvThd
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/data_recv_thd.h"

/**
 * @brief Construct a new DataRecvThd object
 * 
 * @param server_channel the storage server channel
 * @param fp_2_addr_db fp to chunk addr index
 */
DataRecvThd::DataRecvThd(SSLConnection* server_channel,
    AbsDatabase* fp_2_addr_db) {
    server_channel_ = server_channel;
    dedup_util_ = new DedupDetect(fp_2_addr_db);
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();
    finesse_util_ = new FinesseUtil(SUPER_FEATURE_PER_CHUNK,
        FEATURE_PER_CHUNK, FEATURE_PER_SUPER_FEATURE);
    crypto_util_ = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
}

/**
 * @brief Destroy the DataRecvThd object
 * 
 */
DataRecvThd::~DataRecvThd() {
    delete dedup_util_;
    delete finesse_util_;
    delete crypto_util_;
}

/**
 * @brief the main process
 * 
 * @param cur_client the current client var
 */
void DataRecvThd::Run(ClientVar* cur_client) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    uint32_t recv_size = 0;
    string client_ip;
    SendMsgBuffer_t* recv_chunk_buf = &cur_client->_recv_chunk_buf;
    SSL* client_ssl = cur_client->_client_ssl;

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    while (true) {
        // receive data
        if (!server_channel_->ReceiveData(client_ssl, recv_chunk_buf->send_buf, 
            recv_size)) {
            tool::Logging(my_name_.c_str(), "client closed socket connection, thread exits.\n");
            server_channel_->GetClientIp(client_ip, client_ssl);
            server_channel_->ClearAcceptedClientSd(client_ssl);
            break;
        } else {
            gettimeofday(&proc_stime, NULL);
            switch (recv_chunk_buf->header->msg_type) {
                case CLIENT_UPLOAD_CHUNK: {
                    _chunk_batch_num++;
                    this->ProcessChunks(cur_client);
                    break;
                }
                case CLIENT_UPLOAD_RECIPE: {
                    tool::Logging(my_name_.c_str(), "fix: recv recipe.\n");
                    exit(EXIT_FAILURE);
                }
                case CLIENT_UPLOAD_RECIPE_END: {
                    this->ProcessRecipeEnd(cur_client);
                    cur_client->_recv_2_writer_mq->_done = true;
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong recv data type.\n");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);
        }
    }

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread (%s) exits, total proc time: %lf, "
        "total running time: %lf\n", client_ip.c_str(), total_proc_time,
        total_running_time);

    return ;
}

/**
 * @brief process a batch of chunks
 * 
 * @param cur_client the current client var
 */
void DataRecvThd::ProcessChunks(ClientVar* cur_client) {
    SendMsgBuffer_t* recv_chunk_buf = &cur_client->_recv_chunk_buf;
    uint32_t chunk_num = recv_chunk_buf->header->cur_item_num;
    uint32_t offset = 0;
    SendChunkHeader_t* chunk_header_ptr;
    uint8_t* data_buf = recv_chunk_buf->data_buf;
    EVP_MD_CTX* md_ctx = cur_client->_md_ctx;
    uint8_t* chunk_data;

    WrappedChunk_t tmp_chunk; 
    for (size_t i = 0; i < chunk_num; i++) {
        chunk_header_ptr = (SendChunkHeader_t*)(data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        chunk_data = data_buf + offset;

        // generate the chunk fp
        crypto_util_->GenerateHash(md_ctx, chunk_data,
            chunk_header_ptr->size, tmp_chunk.info.fp);

        // perform deduplication
        dedup_util_->DetectDuplicate(&tmp_chunk.info);
        
        this->ProcessRecipe(cur_client, tmp_chunk.info.fp);

        if (tmp_chunk.info.stat == UNIQUE_CHUNK) {
            // unique chunk
            memcpy(tmp_chunk.data, chunk_data, chunk_header_ptr->size);
            tmp_chunk.info.addr.len = chunk_header_ptr->size;

            // compute the feature here
            finesse_util_->ExtractFeature(cur_client->_rabin_ctx,
                tmp_chunk.data, tmp_chunk.info.addr.len,
                tmp_chunk.info.features);
            
            cur_client->_recv_2_writer_mq->Push(tmp_chunk);

            // update stat
            _total_unique_chunk_num++;
            _total_unique_data_size += chunk_header_ptr->size;
        }

        offset += chunk_header_ptr->size;

        // update stat
        _total_logical_chunk_num++;
        _total_logical_data_size += chunk_header_ptr->size;
    }

    return ;
}

/**
 * @brief process the recipe end
 * 
 * @param cur_client the current client var
 */
void DataRecvThd::ProcessRecipeEnd(ClientVar* cur_client) {
    SendMsgBuffer_t* recv_chunk_buf = &cur_client->_recv_chunk_buf;
    ofstream* recipe_write_hdl = &cur_client->_recipe_write_hdl;

    // process the tail recipe
    uint8_t* recipe_buf_base = cur_client->_recipe_batch.buf;
    if (cur_client->_recipe_batch.cnt != 0) {
        recipe_write_hdl->write((char*)recipe_buf_base,
            cur_client->_recipe_batch.cnt * CHUNK_HASH_SIZE);
    }

    recipe_write_hdl->seekp(0, ios_base::beg);
    recipe_write_hdl->write((char*)recv_chunk_buf->data_buf,
        recv_chunk_buf->header->size);
    return ;
}

/**
 * @brief add fp to the recipe
 * 
 * @param cur_client the current client
 * @param fp chunk fp
 */
void DataRecvThd::ProcessRecipe(ClientVar* cur_client, uint8_t* fp) {
    // copy the fp to the recipe buf
    uint8_t* recipe_buf_base = cur_client->_recipe_batch.buf;
    ofstream* recipe_write_hdl = &cur_client->_recipe_write_hdl;

    memcpy(recipe_buf_base + cur_client->_recipe_batch.cnt * CHUNK_HASH_SIZE,
        fp, CHUNK_HASH_SIZE);
    cur_client->_recipe_batch.cnt++;

    if (cur_client->_recipe_batch.cnt % send_recipe_batch_size_ == 0) {
        recipe_write_hdl->write((char*)recipe_buf_base, 
            cur_client->_recipe_batch.cnt * CHUNK_HASH_SIZE);
        cur_client->_recipe_batch.cnt = 0;
    }

    return ;
}