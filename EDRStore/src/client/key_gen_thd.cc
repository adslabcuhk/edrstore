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
 */
KeyGenThd::KeyGenThd(SSLConnection* km_channel,
    pair<int, SSL*> km_conn_record) {
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();

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
        sizeof(KeyGenRet_t) + sizeof(NetworkHead_t));
    recv_buf_.header = (NetworkHead_t*) recv_buf_.send_buf;
    recv_buf_.data_buf = recv_buf_.send_buf + sizeof(NetworkHead_t);

    // the chunk buffer
    chunk_buf_.reserve(send_chunk_batch_size_);

    two_phase_enc_ = new TwoPhaseEnc();

    km_channel_ = km_channel;
    km_conn_record_ = km_conn_record;
    km_ssl_ = km_conn_record.second;

    crypto_util_ = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
    md_ctx = EVP_MD_CTX_new();
}

/**
 * @brief Destroy the KeyGenThd object
 * 
 */
KeyGenThd::~KeyGenThd() {
    delete crypto_util_;
    EVP_MD_CTX_free(md_ctx);
    delete two_phase_enc_;
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
                    this->AddChunkToBuf(tmp_data, output_MQ);
                    break;
                }
                case RECIPE_CHUNK: {
                    // process the tail batch first
                    this->ProcessBatch(output_MQ);
                    output_MQ->Push(tmp_data);
                    km_channel_->Finish(km_conn_record_);
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
void KeyGenThd::AddChunkToBuf(EncFeatureChunk_t& input_chunk,
    AbsMQ<EncFeatureChunk_t>* output_MQ) {
    // buffer this chunk 
    chunk_buf_.push_back(input_chunk);

    // // add the chunk hash and feature to the send buffer: <plaintext fp, features>
    // KeyGenReq_t* cur_key_req = (KeyGenReq_t*) (send_buf_.data_buf + send_buf_.header->size);
    // memcpy(cur_key_req->fp, input_chunk.feature_chunk.chunk.raw_chunk.fp,
    //     CHUNK_HASH_SIZE);    
    // memcpy(cur_key_req->features, input_chunk.feature_chunk.features,
    //     sizeof(uint64_t) * SUPER_FEATURE_PER_CHUNK);
    // send_buf_.header->size += sizeof(KeyGenReq_t);

    // add the feature to the send buffer: <features>
    KeyGenReq_t* cur_key_req = (KeyGenReq_t*) (send_buf_.data_buf + send_buf_.header->size);
    // memcpy(cur_key_req->fp, input_chunk.feature_chunk.chunk.raw_chunk.fp,
    //     CHUNK_HASH_SIZE);

    // for(int i = 0; i < SUPER_FEATURE_PER_CHUNK; i++) {
    //     crypto_util_->GenerateHash(md_ctx, input_chunk.feature_chunk.features[i], 
    //         sizeof(uint64_t), cur_key_req->features[i]);
    // }

    memcpy(cur_key_req->features, input_chunk.feature_chunk.features,
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
void KeyGenThd::ProcessBatch(AbsMQ<EncFeatureChunk_t>* output_MQ) {
    uint32_t recv_size = 0;
    uint32_t cur_batch_size = chunk_buf_.size();
    // EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();

    if (cur_batch_size == 0) {
        return ;
    }

    // send the request to the key mananger
    send_buf_.header->cur_item_num = cur_batch_size;
    send_buf_.header->client_id = config.GetClientID();
    send_buf_.header->msg_type = CLIENT_KEY_GEN;

#ifdef EDR_BREAKDOWN
    gettimeofday(&_key_gen_stime, NULL);
#endif

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

#ifdef EDR_BREAKDOWN
    gettimeofday(&_key_gen_etime, NULL);
    _total_key_gen_time += tool::GetTimeDiff(_key_gen_stime,
        _key_gen_etime);
#endif

    KeyGenRet_t* cur_key_ret = (KeyGenRet_t*) recv_buf_.data_buf;
    for (size_t i = 0; i < cur_batch_size; i++) {
        // // copy the key seed
        // memcpy(chunk_buf_[i].key, cur_key_ret->key_seed, CHUNK_HASH_SIZE);
        // // chunk_buf_[i].seed = cur_key_ret->seed;
        // chunk_buf_[i].seed = 1;

        // seed = plaintext fp
        chunk_buf_[i].seed = this->ConvertFp2Val(chunk_buf_[i].feature_chunk.chunk.raw_chunk.fp, CHUNK_HASH_SIZE);

#ifdef EDR_BREAKDOWN
    gettimeofday(&_key_gen_stime, NULL);
#endif

        // final key = H (sampled plaintext content || key seed)
        uint8_t tmp_generating_key_buf[CHUNK_HASH_SIZE * 2];
        memcpy(tmp_generating_key_buf, chunk_buf_[i].feature_chunk.chunk.raw_chunk.data, CHUNK_HASH_SIZE);
        memcpy(tmp_generating_key_buf + CHUNK_HASH_SIZE, cur_key_ret->key_seed, CHUNK_HASH_SIZE);
        crypto_util_->GenerateHash(md_ctx, tmp_generating_key_buf, CHUNK_HASH_SIZE * 2, chunk_buf_[i].key);

#ifdef EDR_BREAKDOWN
    gettimeofday(&_key_gen_etime, NULL);
    _total_key_gen_time += tool::GetTimeDiff(_key_gen_stime,
        _key_gen_etime);
#endif

#ifdef EDR_BREAKDOWN
        gettimeofday(&_two_enc_stime, NULL);
#endif

        // encrypt the chunk here
        chunk_buf_[i].enc_size = two_phase_enc_->TwoPhaseEncChunk(
            chunk_buf_[i].feature_chunk.chunk.raw_chunk.data,
            chunk_buf_[i].feature_chunk.chunk.raw_chunk.size,
            chunk_buf_[i].key,
            chunk_buf_[i].enc_data
        );

#ifdef EDR_BREAKDOWN
        gettimeofday(&_two_enc_etime, NULL);
        _total_two_enc_time += tool::GetTimeDiff(_two_enc_stime,
            _two_enc_etime);
        _total_two_enc_size += chunk_buf_[i].feature_chunk.chunk.raw_chunk.size;
#endif

        output_MQ->Push(chunk_buf_[i]);
        cur_key_ret++;
    }

    // reset
    send_buf_.header->cur_item_num = 0;
    send_buf_.header->size = 0;
    chunk_buf_.clear();
    // EVP_MD_CTX_free(md_ctx);

    return ;
}

/**
 * @brief convert the fp to value
 * 
 * @param fp chunk fp
 * @param size chunk size
 * @return uint64_t fp value
 */
uint64_t KeyGenThd::ConvertFp2Val(uint8_t* fp, uint32_t size) {
    uint64_t hash_val = 0;
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        hash_val += fp[i];
    }
    return hash_val;
}
