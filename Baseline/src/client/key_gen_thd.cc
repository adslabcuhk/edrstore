/**
 * @file key_gen_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the 
 * @version 0.1
 * @date 2022-06-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/client/key_gen_thd.h"

/**
 * @brief Construct a new KeyGenThd object
 * 
 * @param km_channel the key manager connection channel
 * @param km_conn_record the key manager connection channel
 * @param method_type the key generation method 
 */
KeyGenThd::KeyGenThd(SSLConnection* km_channel,
    pair<int, SSL*> km_conn_record, uint32_t method_type) {
    // for config
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    method_type_ = method_type;

    // send buffer
    send_buf_.send_buf = (uint8_t*) malloc(send_chunk_batch_size_ * 
        sizeof(KeyGenReq_t) + sizeof(NetworkHead_t));
    send_buf_.header = (NetworkHead_t*) send_buf_.send_buf;
    send_buf_.header->client_id = config.GetClientID();
    send_buf_.header->size = 0;
    send_buf_.header->cur_item_num = 0;
    send_buf_.data_buf = send_buf_.send_buf + sizeof(NetworkHead_t);

    // recv buffer
    recv_buf_.send_buf = (uint8_t*) malloc(send_chunk_batch_size_ * 
        sizeof(KeyGenReq_t) + sizeof(NetworkHead_t));
    recv_buf_.header = (NetworkHead_t*) recv_buf_.send_buf;
    recv_buf_.data_buf = recv_buf_.send_buf + sizeof(NetworkHead_t);

    // the chunk buffer
    chunk_buf_.reserve(send_chunk_batch_size_);

    // for key server connection
    km_channel_ = km_channel;
    km_conn_record_ = km_conn_record;
    km_ssl_ = km_conn_record.second;

    mle_util_ = new MLE();
}

/**
 * @brief Destroy the KeyGenThd object
 * 
 */
KeyGenThd::~KeyGenThd() {
    delete mle_util_;
    free(send_buf_.send_buf);
    free(recv_buf_.send_buf);
}

/**
 * @brief the main thread
 * 
 * @param input_MQ the input MQ
 * @param output_MQ the output MQ
 */
void KeyGenThd::Run(AbsMQ<FeatureChunk_t>* input_MQ,
    AbsMQ<EncFeatureChunk_t>* output_MQ) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------

    EncFeatureChunk_t tmp_data;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_data.feature_chunk)) {
            switch (tmp_data.feature_chunk.chunk.type) {
                case NORMAL_CHUNK: {
                    this->AddChunkToBuf(tmp_data.feature_chunk.chunk,
                        output_MQ);
                    break;
                }
                case RECIPE_CHUNK: {
                    // process the tail batch first
                    this->ProcessBatch(output_MQ);
                    if (method_type_ == SERVER_AIDED_MLE || method_type_ == ENC_COMP_MLE) {
                        km_channel_->Finish(km_conn_record_);
                    }
                    output_MQ->Push(tmp_data);
                    break;
                }
                default: {
                    tool::Logging(my_name_.c_str(), "wrong chunk type.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // set the output MQ done 
    output_MQ->_done = true;

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread exits, total running time: %lf\n",
        total_running_time);

    return ;
}

/**
 * @brief add the chunk to the batch plaintext chunk buffer
 * 
 * @param input_chunk the input chunk
 * @param output_MQ the output MQ
 */
void KeyGenThd::AddChunkToBuf(Chunk_t& input_chunk,
    AbsMQ<KeyGen2SelectComp_t>* output_MQ) {
    // buffer this chunk 
    chunk_buf_.push_back(input_chunk);

    // add the chunk hash and feature to the send buffer: <plaintext fp, features>
    KeyGenReq_t* cur_key_req = (KeyGenReq_t*) (send_buf_.data_buf + send_buf_.header->size);
    memcpy(cur_key_req->fp, input_chunk.raw_chunk.fp,
        CHUNK_HASH_SIZE);    
    memset(cur_key_req->features, 0, 
        sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);
    send_buf_.header->size += sizeof(KeyGenReq_t);

    if (chunk_buf_.size() % send_chunk_batch_size_ == 0) {
        this->ProcessBatch(output_MQ);
    }

    return ;
}

/**
 * @brief process a batch of chunks
 * 
 * @param output_MQ the output MQ
 */
void KeyGenThd::ProcessBatch(AbsMQ<KeyGen2SelectComp_t>* output_MQ) {
    uint32_t recv_size = 0;
    uint32_t cur_batch_size = chunk_buf_.size();

    if (cur_batch_size == 0) {
        return ;
    }

    EncFeatureChunk_t tmp_enc_chunk;
    tmp_enc_chunk.feature_chunk.chunk.type = NORMAL_CHUNK;
    switch (method_type_) {
        case ENC_COMP_MLE: {
            ;
        }
        case SERVER_AIDED_MLE: {
            // send the request to the key mananger
            send_buf_.header->cur_item_num = cur_batch_size;
            send_buf_.header->client_id = config.GetClientID();
            send_buf_.header->msg_type = CLIENT_KEY_GEN;

            if (!km_channel_->SendData(km_ssl_, send_buf_.send_buf,
                send_buf_.header->size + sizeof(NetworkHead_t))) {
                tool::Logging(my_name_.c_str(), "send the key gen batch error.\n");
                exit(EXIT_FAILURE);
            }

            if (!km_channel_->ReceiveData(km_ssl_, recv_buf_.send_buf,
                recv_size)) {
                tool::Logging(my_name_.c_str(), "recv the key gen batch error.\n");
                exit(EXIT_FAILURE);
            }

            KeyGenRet_t* cur_key_ret = (KeyGenRet_t*) recv_buf_.data_buf;
            for (size_t i = 0; i < cur_batch_size; i++) {
                // copy the key and seed
                memcpy(tmp_enc_chunk.key, cur_key_ret->key, CHUNK_HASH_SIZE);
                cur_key_ret++;

                // encrypt the chunk with MLE
                mle_util_->EncChunk(chunk_buf_[i].raw_chunk.data, chunk_buf_[i].raw_chunk.size,
                    tmp_enc_chunk.key, tmp_enc_chunk.enc_data);
                tmp_enc_chunk.enc_size = chunk_buf_[i].raw_chunk.size;

                memcpy(&tmp_enc_chunk.feature_chunk.chunk.raw_chunk,
                    &chunk_buf_[i].raw_chunk,
                    sizeof(RawChunk_t));
                output_MQ->Push(tmp_enc_chunk);
            }
            break;
        }
        case PLAIN: {
            // directly add the chunk to the output MQ
            for (size_t i = 0; i < cur_batch_size; i++) {
                // use an all '0' virtual key
                memset(tmp_enc_chunk.key, 0, CHUNK_HASH_SIZE);

                // directly copy the plaintext chunk
                memcpy(tmp_enc_chunk.enc_data, chunk_buf_[i].raw_chunk.data,
                    chunk_buf_[i].raw_chunk.size);
                tmp_enc_chunk.enc_size = chunk_buf_[i].raw_chunk.size;

                memcpy(&tmp_enc_chunk.feature_chunk.chunk.raw_chunk,
                    &chunk_buf_[i].raw_chunk,
                    sizeof(RawChunk_t));
                output_MQ->Push(tmp_enc_chunk);
            }
            break;
        }
    }

    // reset
    send_buf_.header->cur_item_num = 0;
    send_buf_.header->size = 0;
    chunk_buf_.clear();

    return ;
}