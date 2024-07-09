/**
 * @file data_decoder.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces of DataDecoderThd
 * @version 0.1
 * @date 2022-07-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/data_decoder_thd.h"

/**
 * @brief Construct a new DataDecoderThd object
 * 
 * @param server_channel server channel
 */
DataDecoderThd::DataDecoderThd(SSLConnection* server_channel) {
    // for config
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();

    server_channel_ = server_channel;
    delta_comp_ = new DeltaComp();
    compress_util_ = new CompressUtil(COMPRESSION_TYPE, COMPRESSION_LEVEL);
}

/**
 * @brief Destroy the DataDecodeThd object
 * 
 */
DataDecoderThd::~DataDecoderThd() {
    delete delta_comp_;
    delete compress_util_;
}

/**
 * @brief the main process
 * 
 * @param cur_client current client
 */
void DataDecoderThd::Run(ClientVar* cur_client) {
    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    AbsMQ<Reader2Decoder_t>* input_MQ = cur_client->_reader_2_decoder_mq;
    SendMsgBuffer_t* send_chunk_buf = &cur_client->_send_chunk_buf;

    struct timeval stime;
    struct timeval etime;
    double total_running_time = 0;

    struct timeval proc_stime;
    struct timeval proc_etime;
    double total_proc_time = 0;

    gettimeofday(&stime, NULL);
    // -------- main process --------
    
    Reader2Decoder_t tmp_raw_chunk;
    string client_ip;
    while (true) {
        // extract a chunk from the MQ
        if (input_MQ->_done && input_MQ->IsEmpty()) {
            tool::Logging(my_name_.c_str(), "no chunk in the MQ, all jobs are done.\n");
            break;
        }

        if (input_MQ->Pop(tmp_raw_chunk)) {
            gettimeofday(&proc_stime, NULL);
            this->DecodeChunk(&tmp_raw_chunk, cur_client);
            gettimeofday(&proc_etime, NULL);
            total_proc_time += tool::GetTimeDiff(proc_stime, proc_etime);

            if (send_chunk_buf->header->cur_item_num % send_chunk_batch_size_ == 0) {
                this->SendChunks(cur_client);
            }
        }
    }

    if (send_chunk_buf->header->cur_item_num != 0) {
        this->SendChunks(cur_client);
    }
    this->SendEnd(cur_client, client_ip);

    gettimeofday(&etime, NULL);
    total_running_time += tool::GetTimeDiff(stime, etime);

    tool::Logging(my_name_.c_str(), "thread (%s) exits, total proc time: %lf, "
        "total running time: %lf\n", client_ip.c_str(), total_proc_time,
        total_running_time);

    return ;
}

/**
 * @brief decode a chunk
 * 
 * @param raw_chunk raw chunk
 * @param cur_client current client
 */
void DataDecoderThd::DecodeChunk(Reader2Decoder_t* raw_chunk,
    ClientVar* cur_client) {
    SendMsgBuffer_t* send_chunk_buf = &cur_client->_send_chunk_buf;

    // step-1: perform decompression if necessary
    SendChunk_t real_input_chunk;
    if ((raw_chunk->input_chunk.header.type == COMP_BASE_CHUNK) ||
        (raw_chunk->input_chunk.header.type == COMP_DELTA_CHUNK)) {
        // need decompression
        compress_util_->DecompressOneChunk(raw_chunk->input_chunk.data,
            raw_chunk->input_chunk.header.size, real_input_chunk.data,
            &real_input_chunk.header.size);
    } else {
        memcpy(real_input_chunk.data, raw_chunk->input_chunk.data,
            raw_chunk->input_chunk.header.size);
        real_input_chunk.header.size = raw_chunk->input_chunk.header.size;
    }

    // step-2: perform delta decoding if necessary
    if ((raw_chunk->input_chunk.header.type == COMP_DELTA_CHUNK) ||
        (raw_chunk->input_chunk.header.type == UNCOMP_DELTA_CHUNK)) {
        // need delta decoding
        SendChunk_t real_base_chunk;
        if ((raw_chunk->base_chunk.header.type == UNCOMP_BASE_CHUNK) ||
            (raw_chunk->base_chunk.header.type == COMP_BASE_CHUNK)) {
            if (raw_chunk->base_chunk.header.type == COMP_BASE_CHUNK) {
                // need decompression
                compress_util_->DecompressOneChunk(raw_chunk->base_chunk.data,
                    raw_chunk->base_chunk.header.size, real_base_chunk.data,
                    &real_base_chunk.header.size);
            } else {
                memcpy(real_base_chunk.data, raw_chunk->base_chunk.data,
                    raw_chunk->base_chunk.header.size);
                real_base_chunk.header.size =
                    raw_chunk->base_chunk.header.size;
            }
        } else {
            // exists multi-level delta
            tool::Logging(my_name_.c_str(), "exists multi-level delta.\n");
            exit(EXIT_FAILURE);
        }

        // perform delta decoding
        SendChunk_t output_chunk;
        output_chunk.header.size = delta_comp_->DeltaDecode(real_base_chunk.data,
            real_base_chunk.header.size, real_input_chunk.data,
            real_input_chunk.header.size, output_chunk.data);
        
        // write data to the send buf
        output_chunk.header.type = NORMAL_CHUNK;
        memcpy(send_chunk_buf ->data_buf + send_chunk_buf->header->size,
            &output_chunk.header, sizeof(SendChunkHeader_t));
        send_chunk_buf->header->size += sizeof(SendChunkHeader_t);
        memcpy(send_chunk_buf->data_buf + send_chunk_buf->header->size,
            output_chunk.data, output_chunk.header.size);
        send_chunk_buf->header->size += output_chunk.header.size;
        send_chunk_buf->header->cur_item_num++;
    } else {
        // write data to the send buf
        real_input_chunk.header.type = NORMAL_CHUNK;
        memcpy(send_chunk_buf->data_buf + send_chunk_buf->header->size,
            &real_input_chunk.header, sizeof(SendChunkHeader_t));
        send_chunk_buf->header->size += sizeof(SendChunkHeader_t);
        memcpy(send_chunk_buf->data_buf + send_chunk_buf->header->size,
            real_input_chunk.data, real_input_chunk.header.size);
        send_chunk_buf->header->size += real_input_chunk.header.size;
        send_chunk_buf->header->cur_item_num++;
    }

    return ;
}

/**
 * @brief send a batch of chunk
 * 
 * @param cur_client current client 
 */
void DataDecoderThd::SendChunks(ClientVar* cur_client) {
    SendMsgBuffer_t* send_chunk_buf = &cur_client->_send_chunk_buf;
    send_chunk_buf->header->msg_type = SERVER_RESTORE_CHUNK;
    if (!server_channel_->SendData(cur_client->_client_ssl,
        send_chunk_buf->send_buf,
        send_chunk_buf->header->size + sizeof(NetworkHead_t))) {
        tool::Logging(my_name_.c_str(), "send the restore chunk batch error.\n");
        exit(EXIT_FAILURE);
    }

    // clear the current send chunk buffer
    send_chunk_buf->header->cur_item_num = 0;
    send_chunk_buf->header->size = 0;

    return ;
}

/**
 * @brief send the end flag 
 * 
 * @param cur_client current client
 * @param client_ip client ip
 */
void DataDecoderThd::SendEnd(ClientVar* cur_client, string& client_ip) {
    SendMsgBuffer_t* send_chunk_buf = &cur_client->_send_chunk_buf;
    send_chunk_buf->header->msg_type = SERVER_RESTORE_FINAL; 
    SSL* client_ssl = cur_client->_client_ssl;

    if (!server_channel_->SendData(client_ssl, send_chunk_buf->send_buf,
        sizeof(NetworkHead_t))) {
        tool::Logging(my_name_.c_str(), "send the restore end flag error.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t recv_size = 0;
    if (!server_channel_->ReceiveData(client_ssl, send_chunk_buf->send_buf,
        recv_size)) {
        tool::Logging(my_name_.c_str(), "client closed socket connection, thread exits.\n");
        server_channel_->GetClientIp(client_ip, client_ssl);
        server_channel_->ClearAcceptedClientSd(client_ssl);
    } else {
        tool::Logging(my_name_.c_str(), "client closed the connection error.\n");
        exit(EXIT_FAILURE);
    }

    return ;
}