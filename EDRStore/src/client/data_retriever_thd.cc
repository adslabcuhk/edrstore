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
    two_phase_enc_ = new TwoPhaseEnc();
    method_type_ = method_type;

    // for decompression
    comp_pad_ = new CompPad();

    // for delta compression
    delta_comp_ = new DeltaComp();
}

/**
 * @brief Destroy the DataRetrieverThd object
 * 
 */
DataRetrieverThd::~DataRetrieverThd() {
    key_recipe_hdl_.close();
    free(recv_chunk_buf_.send_buf);
    free(key_recipe_buf_.buf);
    delete two_phase_enc_;
    delete comp_pad_;
    delete delta_comp_;
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
                    switch (method_type_) {
                        case ONLY_SIMILAR_ENC: {
                            this->OnlyEncMode(output_MQ);
                            break;
                        }
                        case SIMILAR_ENC_COMP: {
                            this->EncCompMode(output_MQ);
                            break;
                        }
                        case FULL_EDR: {
                            this->FullEDR(output_MQ);
                            break;
                        }
                        default: {
                            tool::Logging(my_name_.c_str(), "wrong restore method.\n");
                            exit(EXIT_FAILURE);
                        }
                    }
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
 * @brief process a batch of chunks (Full EDR)
 * 
 * @param output_MQ output MQ
 */
void DataRetrieverThd::FullEDR(AbsMQ<Retriever2Writer_t>* output_MQ) {
    uint32_t cur_chunk_num = recv_chunk_buf_.header->cur_item_num;
    SendChunkHeader_t* cur_header;
    uint8_t* cur_data;

    SendChunk_t tmp_restore_chunk;
    SendChunk_t tmp_input_chunk;
    SendChunk_t tmp_base_chunk;
    SendChunk_t tmp_enc_restore_chunk;
    size_t offset = 0;
    
    for (size_t i = 0; i < cur_chunk_num; i++) {
        cur_header = (SendChunkHeader_t*)(recv_chunk_buf_.data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        cur_data = recv_chunk_buf_.data_buf + offset;

        if (remain_recipe_num_ == 0) {
            // fetch a new batch of key recipes
            this->FetchKeyRecipe();
        }

        KeyRecipe_t* tmp_key_recipe = (KeyRecipe_t*)(key_recipe_buf_.buf +
            key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
        switch (cur_header->type) {
            case COMP_NORMAL_CHUNK: {
                this->ProcCompChunk(cur_data, cur_header->size,
                    &tmp_restore_chunk, tmp_key_recipe);
                break;
            }
            case UNCOMP_NORMAL_CHUNK: {
                this->ProcUncompChunk(cur_data, cur_header->size,
                    &tmp_restore_chunk, tmp_key_recipe);
                break;
            }
            case CACHE_RESTORE_DELTA: {
                // store the delta chunk in the tmp_input_chunk
                memcpy(tmp_input_chunk.data, cur_data, cur_header->size);
                tmp_input_chunk.header.size = cur_header->size;

                // -------- read the compressed base chunk --------
                offset += cur_header->size;

                cur_header =
                    (SendChunkHeader_t*)(recv_chunk_buf_.data_buf + offset);
                offset += sizeof(SendChunkHeader_t);
                cur_data = recv_chunk_buf_.data_buf + offset;
                this->ProcRestoreBaseChunk(cur_data, cur_header->size,
                    &tmp_base_chunk, tmp_key_recipe);

                tmp_enc_restore_chunk.header.size = delta_comp_->DeltaDecode(
                    tmp_base_chunk.data,
                    tmp_base_chunk.header.size,
                    tmp_input_chunk.data,
                    tmp_input_chunk.header.size,
                    tmp_enc_restore_chunk.data);
                
                this->ProcUncompChunk(tmp_enc_restore_chunk.data,
                    tmp_enc_restore_chunk.header.size,
                    &tmp_restore_chunk, tmp_key_recipe);

                break;
            }
            case MULTI_LEVEL_DELTA_CHUNK: {
                // TODO: directly ignore multi-level delta
                memcpy(tmp_restore_chunk.data, cur_data, cur_header->size);
                tmp_restore_chunk.header.size = cur_header->size;
                break;
            }

            default: {
                tool::Logging(my_name_.c_str(), "wrong recv chunk type.\n");
                exit(EXIT_FAILURE);
            }
        }
        
        output_MQ->Push(tmp_restore_chunk);
        offset += cur_header->size;

        key_recipe_buf_.cnt++;
        remain_recipe_num_--;

        _total_recv_chunk_num++;
        _total_recv_data_size += tmp_restore_chunk.header.size;
    }

    return ;
}

/**
 * @brief process a batch of chunks (only enc)
 * 
 * @param output_MQ output MQ
 */
void DataRetrieverThd::OnlyEncMode(AbsMQ<Retriever2Writer_t>* output_MQ) {
    uint32_t cur_chunk_num = recv_chunk_buf_.header->cur_item_num;
    SendChunkHeader_t* cur_header;
    uint8_t* cur_data;

    SendChunk_t tmp_restore_chunk;
    size_t offset = 0;
    
    for (size_t i = 0; i < cur_chunk_num; i++) {
        cur_header = (SendChunkHeader_t*)(recv_chunk_buf_.data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        cur_data = recv_chunk_buf_.data_buf + offset;

        if (remain_recipe_num_ == 0) {
            // fetch a new batch of key recipes
            this->FetchKeyRecipe();
        }

        KeyRecipe_t* tmp_key_recipe = (KeyRecipe_t*)(key_recipe_buf_.buf +
            key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
        this->ProcUncompChunk(cur_data, cur_header->size,
            &tmp_restore_chunk, tmp_key_recipe);
        
        output_MQ->Push(tmp_restore_chunk);
        offset += cur_header->size;

        key_recipe_buf_.cnt++;
        remain_recipe_num_--;

        _total_recv_chunk_num++;
        _total_recv_data_size += tmp_restore_chunk.header.size;
    }

    return ;
}

/**
 * @brief process a batch of chunks (comp + enc)
 * 
 * @param output_MQ output MQ
 */
void DataRetrieverThd::EncCompMode(AbsMQ<Retriever2Writer_t>* output_MQ) {
    uint32_t cur_chunk_num = recv_chunk_buf_.header->cur_item_num;
    SendChunkHeader_t* cur_header;
    uint8_t* cur_data;

    SendChunk_t tmp_restore_chunk;
    size_t offset = 0;
    
    for (size_t i = 0; i < cur_chunk_num; i++) {
        cur_header = (SendChunkHeader_t*)(recv_chunk_buf_.data_buf + offset);
        offset += sizeof(SendChunkHeader_t);
        cur_data = recv_chunk_buf_.data_buf + offset;

        if (remain_recipe_num_ == 0) {
            // fetch a new batch of key recipes
            this->FetchKeyRecipe();
        }

        KeyRecipe_t* tmp_key_recipe = (KeyRecipe_t*)(key_recipe_buf_.buf +
            key_recipe_buf_.cnt * sizeof(KeyRecipe_t));
        this->ProcCompChunk(cur_data, cur_header->size,
            &tmp_restore_chunk, tmp_key_recipe);
        
        output_MQ->Push(tmp_restore_chunk);
        offset += cur_header->size;

        key_recipe_buf_.cnt++;
        remain_recipe_num_--;

        _total_recv_chunk_num++;
        _total_recv_data_size += tmp_restore_chunk.header.size;
    }

    return ;
}

/**
 * @brief process a comp chunk
 * 
 * @param input_chunk input chunk
 * @param size chunk size
 * @param output_chunk output chunk
 * @param key_recipe key recipe
 */
void DataRetrieverThd::ProcCompChunk(uint8_t* input_chunk, uint32_t size,
    SendChunk_t* output_chunk, KeyRecipe_t* key_recipe) {
    uint8_t tmp_dec_buf[ENC_MAX_CHUNK_SIZE];

    // first decrypt, then decompression
    uint32_t w_padding_size;
    w_padding_size = two_phase_enc_->TwoPhaseDecChunk(input_chunk,
        size, key_recipe->key, tmp_dec_buf);
    
    // decompression with real size
    output_chunk->header.size = comp_pad_->DecompressWithPad(
        tmp_dec_buf, w_padding_size, output_chunk->data);
    
    return ;
}

/**
 * @brief proc an un-comp chunk
 * 
 * @param input_chunk input chunk
 * @param size chunk size
 * @param output_chunk output chunk
 * @param key_recipe key recipe
 */
void DataRetrieverThd::ProcUncompChunk(uint8_t* input_chunk, uint32_t size,
    SendChunk_t* output_chunk, KeyRecipe_t* key_recipe) {
    // directly decrypt
    output_chunk->header.size = two_phase_enc_->TwoPhaseDecChunk(input_chunk,
        size, key_recipe->key, output_chunk->data);

    return ;
}

/**
 * @brief process restore base chunk
 * 
 * @param input_chunk input chunk
 * @param size chunk size
 * @param restore_chunk output chunk
 * @param key_recipe key recipe
 */
void DataRetrieverThd::ProcRestoreBaseChunk(uint8_t* input_chunk,
    uint32_t size, SendChunk_t* restore_chunk, KeyRecipe_t* key_recipe) {
    uint8_t un_enc_comp_base[ENC_MAX_CHUNK_SIZE];
    uint8_t base_chunk[ENC_MAX_CHUNK_SIZE];
    uint32_t w_padding_size;
    uint32_t wo_padding_size;

    // decrypt the encrypted compressed base chunk
    w_padding_size = two_phase_enc_->TwoPhaseDecChunk(input_chunk,
        size, key_recipe->key, un_enc_comp_base);
    
    // decompress the compressed base chunk
    wo_padding_size = comp_pad_->DecompressWithPad(un_enc_comp_base,
        w_padding_size, base_chunk);

    // re-encrypt the base chunk
    restore_chunk->header.size = two_phase_enc_->TwoPhaseEncChunk(
        base_chunk, wo_padding_size, key_recipe->key, restore_chunk->data);

    return ;
}