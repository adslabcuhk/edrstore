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
    fp_2_addr_db_ = fp_2_addr_db;
    dedup_util_ = new DedupDetect(fp_2_addr_db_);
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
                    break;
                }
                case CLIENT_UPLOAD_FEATURE: {
                    this->ProcessEvictFeature(cur_client);
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

    cur_client->_recv_2_dual_mq->_done = true;
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
    AbsMQ<WrappedChunk_t>* output_MQ = cur_client->_recv_2_dual_mq;
    uint32_t recv_chunk_num = recv_chunk_buf->header->cur_item_num;
    uint32_t offset = 0;
    SendChunkHeader_t* chunk_header_ptr;
    uint8_t* data_buf = recv_chunk_buf->data_buf;
    EVP_MD_CTX* md_ctx = cur_client->_md_ctx;
    uint8_t* chunk_data;

    WrappedChunk_t tmp_chunk;
    uint32_t cur_chunk_num = 0;
    uint8_t dual_fp_buf[CHUNK_HASH_SIZE * 2];

    while (cur_chunk_num != recv_chunk_num) {
        chunk_header_ptr = (SendChunkHeader_t*)(data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        chunk_data = data_buf + offset;

        switch (chunk_header_ptr->type) {
            case FULL_EDR_CACHE_CHUNK: {
                tmp_chunk.info.size = chunk_header_ptr->size;

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_stime, NULL);
#endif

                // uncompressed cached chunk + compressed normal chunk
                crypto_util_->GenerateHash(md_ctx, chunk_data,
                    tmp_chunk.info.size, dual_fp_buf);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_etime, NULL);
                _total_cipher_fp_time += tool::GetTimeDiff(_cipher_fp_stime,
                    _cipher_fp_etime);
                _total_cipher_fp_data_size += tmp_chunk.info.size;
#endif

                // copy the data to the tmp chunk
                memcpy(tmp_chunk.data, chunk_data, tmp_chunk.info.size);

                // copy the cipher feature from the client
                memcpy(tmp_chunk.info.features, chunk_header_ptr->cipher_features,
                    sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);

                // mark this chunk is for cache insertion
                tmp_chunk.info.stat = CACHE_INSERT_CHUNK;

                offset += chunk_header_ptr->size;

                // -------- read the later compressed normal chunk --------
                chunk_header_ptr = (SendChunkHeader_t*)(data_buf + offset);
                offset += sizeof(SendChunkHeader_t);
                chunk_data = data_buf + offset;

                uint32_t comp_size = chunk_header_ptr->size;

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_stime, NULL);
#endif

                crypto_util_->GenerateHash(md_ctx, chunk_data, 
                    comp_size, dual_fp_buf + CHUNK_HASH_SIZE);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_etime, NULL);
                _total_cipher_fp_time += tool::GetTimeDiff(_cipher_fp_stime,
                    _cipher_fp_etime);
                _total_cipher_fp_data_size += comp_size;
#endif


#ifdef EDR_BREAKDOWN
                gettimeofday(&_dual_fp_stime, NULL);
#endif
                // generate dual fp
                crypto_util_->GenerateHash(md_ctx, dual_fp_buf,
                    CHUNK_HASH_SIZE * 2, tmp_chunk.info.fp);
#ifdef EDR_BREAKDOWN
                gettimeofday(&_dual_fp_etime, NULL);
                _total_dual_fp_time += tool::GetTimeDiff(_dual_fp_stime,
                    _dual_fp_etime);
                _total_dual_fp_data_size += CHUNK_HASH_SIZE * 2;
#endif

                // insert the chunk for cache
                output_MQ->Push(tmp_chunk);

                // prepare for the compressed chunk
                tmp_chunk.info.size = chunk_header_ptr->size;

                // insert chunk to the next thd for dedup
                // copy the data to the tmp chunk
                memcpy(tmp_chunk.data, chunk_data, tmp_chunk.info.size);
                tmp_chunk.info.stat = CHUNK_PAIR;
                output_MQ->Push(tmp_chunk);

                this->ProcessRecipe(cur_client, tmp_chunk.info.fp);

                offset += chunk_header_ptr->size;

                // update stat
                _total_logical_chunk_num++;
                _total_logical_data_size += tmp_chunk.info.size;

                break;
            }
            case FULL_EDR_UNCOMPRESS_CHUNK: {
                // it is a normal chunk
                tmp_chunk.info.size = chunk_header_ptr->size;
                memcpy(dual_fp_buf + CHUNK_HASH_SIZE, chunk_header_ptr->compressed_fp,
                    CHUNK_HASH_SIZE);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_stime, NULL);
#endif
                crypto_util_->GenerateHash(md_ctx, chunk_data,
                    tmp_chunk.info.size, dual_fp_buf);
#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_etime, NULL);
                _total_cipher_fp_time += tool::GetTimeDiff(_cipher_fp_stime,
                    _cipher_fp_etime);
                _total_cipher_fp_data_size += tmp_chunk.info.size;
#endif

#ifdef EDR_BREAKDOWN
                gettimeofday(&_dual_fp_stime, NULL);
#endif
                // generate the dual-finerprint
                crypto_util_->GenerateHash(md_ctx, dual_fp_buf, 
                    CHUNK_HASH_SIZE * 2, tmp_chunk.info.fp);
#ifdef EDR_BREAKDOWN
                gettimeofday(&_dual_fp_etime, NULL);
                _total_dual_fp_time += tool::GetTimeDiff(_dual_fp_stime,
                    _dual_fp_etime);
                _total_dual_fp_data_size += CHUNK_HASH_SIZE * 2;
#endif


                // insert into the next thd for dedup
                tmp_chunk.info.stat = SINGLE_CHUNK;
                memcpy(tmp_chunk.data, chunk_data, tmp_chunk.info.size);
                // copy the cipher feature from the client
                memcpy(tmp_chunk.info.features, chunk_header_ptr->cipher_features,
                    sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);
                output_MQ->Push(tmp_chunk);

                this->ProcessRecipe(cur_client, tmp_chunk.info.fp);

                offset += chunk_header_ptr->size;

                // update stat
                _total_logical_chunk_num++;
                _total_logical_data_size += tmp_chunk.info.size;

                break;
            }
            case NORMAL_CHUNK: {
                // it is a normal chunk
                tmp_chunk.info.size = chunk_header_ptr->size;

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_stime, NULL);
#endif

                crypto_util_->GenerateHash(md_ctx, chunk_data,
                    tmp_chunk.info.size, tmp_chunk.info.fp);

                memset(tmp_chunk.info.addr.compressed_fp, 0, CHUNK_HASH_SIZE);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_cipher_fp_etime, NULL);
                _total_cipher_fp_time += tool::GetTimeDiff(_cipher_fp_stime,
                    _cipher_fp_etime);
                _total_cipher_fp_data_size += tmp_chunk.info.size;
#endif

#ifdef EDR_BREAKDOWN
                gettimeofday(&_dedup_stime, NULL);
#endif

                // perform deduplication
                dedup_util_->DetectDuplicate(&tmp_chunk.info);

#ifdef EDR_BREAKDOWN
                gettimeofday(&_dedup_etime, NULL);
                _total_dedup_time += tool::GetTimeDiff(_dedup_stime, _dedup_etime);
#endif

                this->ProcessRecipe(cur_client, tmp_chunk.info.fp);

                if (tmp_chunk.info.stat == UNIQUE_CHUNK) {
                    // unique chunk
                    memcpy(tmp_chunk.data, chunk_data, tmp_chunk.info.size);

#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cipher_feature_stime, NULL);
#endif

                    // compute the feature here
                    finesse_util_->ExtractFeature(cur_client->_rabin_ctx,
                        tmp_chunk.data, tmp_chunk.info.size,
                        tmp_chunk.info.features);

#ifdef EDR_BREAKDOWN
                    gettimeofday(&_cipher_feature_etime, NULL);
                    _total_cipher_feature_time += tool::GetTimeDiff(
                        _cipher_feature_stime, _cipher_feature_etime);
                    _total_cipher_feature_data_size += tmp_chunk.info.size;
#endif
                
                    output_MQ->Push(tmp_chunk);

                    // update stat
                    _total_unique_chunk_num++;
                    _total_unique_data_size += tmp_chunk.info.size;
                }
                offset += chunk_header_ptr->size;

                // update stat
                _total_logical_chunk_num++;
                _total_logical_data_size += tmp_chunk.info.size;

                break;
            }
            default: {
                tool::Logging(my_name_.c_str(), "recv chunk type error.\n");
                exit(EXIT_FAILURE);
            }
        }
        cur_chunk_num++;
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

/**
 * @brief process evict features
 * 
 * @param cur_client current client
 */
void DataRecvThd::ProcessEvictFeature(ClientVar* cur_client) {
    SendMsgBuffer_t* recv_chunk_buf = &cur_client->_recv_chunk_buf;
    AbsMQ<WrappedChunk_t>* output_MQ = cur_client->_recv_2_dual_mq;
    uint32_t recv_feature_num = recv_chunk_buf->header->cur_item_num;
    uint32_t proc_feature_num = 0;
    
    WrappedChunk_t tmp_feature_chunk;
    tmp_feature_chunk.info.stat = CACHE_EVICT_CHUNK;
    while (proc_feature_num != recv_feature_num) {
        if ((recv_feature_num - proc_feature_num) > feature_batch_size_) {
            memcpy(tmp_feature_chunk.data,
                recv_chunk_buf->data_buf + proc_feature_num * sizeof(uint64_t),
                feature_batch_size_ * sizeof(uint64_t));
            tmp_feature_chunk.info.size = feature_batch_size_;
            proc_feature_num += feature_batch_size_;
        } else {
            memcpy(tmp_feature_chunk.data,
                recv_chunk_buf->data_buf + proc_feature_num * sizeof(uint64_t),
                (recv_feature_num - proc_feature_num) * sizeof(uint64_t));
            tmp_feature_chunk.info.size =
                recv_feature_num - proc_feature_num;
            proc_feature_num += (recv_feature_num - proc_feature_num);
        }

        output_MQ->Push(tmp_feature_chunk);
    }

    return ;
}