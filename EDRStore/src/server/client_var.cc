/**
 * @file curClient.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces 
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/client_var.h"

/**
 * @brief Construct a new Client Var object
 * 
 * @param client_id the client ID
 * @param client_ssl the client SSL
 * @param opt_type the operation type (upload / download)
 * @param recipe_path the file recipe path
 */
ClientVar::ClientVar(uint32_t client_id, SSL* client_ssl,
    int opt_type, string& recipe_path) {
    // basic info
    _client_id = client_id;
    _client_ssl = client_ssl;
    opt_type_ = opt_type;
    recipe_path_ = recipe_path;
    my_name_ = my_name_ + "-" + to_string(client_id);

    // config
    send_chunk_batch_size_ = config.GetSendChunkBatchSize();
    send_recipe_batch_size_ = config.GetSendRecipeBatchSize();

    // init the container cache
    _container_cache = new ReadCache(config.GetContainerCacheSize(),
        MAX_CONTAINER_SIZE);
    _inform_cache = new InformCache(_client_id);

    switch (opt_type_) {
        case UPLOAD_OPT: {
            this->InitUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->InitDownloadBuffer();
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong client opt type.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Destroy the Client Var object
 * 
 */
ClientVar::~ClientVar() {
    delete _container_cache;
    *_total_cache_size = _inform_cache->DeleteEvictChunk();
    delete _inform_cache;
    switch (opt_type_) {
        case UPLOAD_OPT: {
            this->DestroyUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->DestroyDownloadBuffer();
            break;
        }
    }
}

/**
 * @brief init the var related to the upload
 * 
 */
void ClientVar::InitUploadBuffer() {
    // assign a random id to the container
    tool::CreateUUID(_cur_container.id, CONTAINER_ID_LENGTH);
    _cur_container.cur_size = 0;

    // init the recv buffer
    _recv_chunk_buf.send_buf = (uint8_t*) malloc(2 * send_chunk_batch_size_ * 
        sizeof(SendChunk_t) + sizeof(NetworkHead_t));
    _recv_chunk_buf.header = (NetworkHead_t*) _recv_chunk_buf.send_buf;
    _recv_chunk_buf.header->client_id = _client_id;
    _recv_chunk_buf.header->size = 0;
    _recv_chunk_buf.header->cur_item_num = 0;
    _recv_chunk_buf.data_buf = _recv_chunk_buf.send_buf + sizeof(NetworkHead_t);
    
    // rabin fp
    rabin_util_ = new RabinFPUtil(config.GetSimilarSlidingWinSize());
    rabin_util_->NewCtx(_rabin_ctx);

    // prepare the crypto
    _md_ctx = EVP_MD_CTX_new();
    _cipher_ctx = EVP_CIPHER_CTX_new();

    // init the file recipe handler
    _recipe_write_hdl.open(recipe_path_, ios_base::trunc | ios_base::binary);
    if (!_recipe_write_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init recipe file: %s\n",
            recipe_path_.c_str());
        exit(EXIT_FAILURE);
    }
    FileRecipeHead_t v_recipe_end;
    _recipe_write_hdl.write((char*)&v_recipe_end, sizeof(FileRecipeHead_t));

    // init the recipe buf
    _recipe_batch.buf = (uint8_t*) malloc(send_recipe_batch_size_ * CHUNK_HASH_SIZE);
    _recipe_batch.cnt = 0;

    // init the MQ
    // _recv_2_comp_mq = wrapped_chunk_mq_factory_.CreateMQ(MQ_TYPE_,
    //     CHUNK_QUEUE_SIZE);
    // _comp_2_writer_mq = wrapped_chunk_mq_factory_.CreateMQ(MQ_TYPE_,
    //     CHUNK_QUEUE_SIZE);
    _recv_2_dual_mq = wrapped_chunk_mq_factory_.CreateMQ(MQ_TYPE_,
        CHUNK_QUEUE_SIZE);
    _dual_2_comp_mq = wrapped_chunk_mq_factory_.CreateMQ(MQ_TYPE_,
        CHUNK_QUEUE_SIZE);
    _comp_2_writer_mq = wrapped_chunk_mq_factory_.CreateMQ(MQ_TYPE_,
        CHUNK_QUEUE_SIZE);

    return ;
}

/**
 * @brief init the var related to the download
 * 
 */
void ClientVar::DestroyUploadBuffer() {
    if (_recipe_write_hdl.is_open()) {
        _recipe_write_hdl.close();
    }
    free(_recipe_batch.buf);
    rabin_util_->FreeCtx(_rabin_ctx);
    free(_recv_chunk_buf.send_buf);
    EVP_MD_CTX_free(_md_ctx);
    EVP_CIPHER_CTX_free(_cipher_ctx);
    // delete _recv_2_comp_mq;
    // delete _comp_2_writer_mq;
    delete _recv_2_dual_mq;
    delete _dual_2_comp_mq;
    delete _comp_2_writer_mq;
    delete rabin_util_;
    return ;
}

/**
 * @brief init var related to the download
 * 
 */
void ClientVar::InitDownloadBuffer() {
    // init the send chunk buffer
    _send_chunk_buf.send_buf = (uint8_t*) malloc(2 * send_chunk_batch_size_ * 
        sizeof(SendChunk_t) + sizeof(NetworkHead_t));
    _send_chunk_buf.header = (NetworkHead_t*) _send_chunk_buf.send_buf;
    _send_chunk_buf.header->client_id = _client_id;
    _send_chunk_buf.header->cur_item_num = 0;
    _send_chunk_buf.header->size = 0;
    _send_chunk_buf.data_buf = _send_chunk_buf.send_buf + sizeof(NetworkHead_t);

    // init recipe read buf
    _read_recipe_buf = (uint8_t*) malloc(send_recipe_batch_size_ *
        CHUNK_HASH_SIZE);

    // init the recipe handler
    _recipe_read_hdl.open(recipe_path_, ios_base::in | ios_base::binary);
    if (!_recipe_read_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot init the file recipe: %s.\n",
            recipe_path_.c_str());
        exit(EXIT_FAILURE);
    }

    // init the MQ
    _reader_2_decoder_mq = reader_2_decoder_mq_factory_.CreateMQ(MQ_TYPE_,
        CHUNK_QUEUE_SIZE);

    return ;
}

/**
 * @brief destroy the var related to the download
 * 
 */
void ClientVar::DestroyDownloadBuffer() {
    if (_recipe_read_hdl.is_open()) {
        _recipe_read_hdl.close();
    }
    free(_send_chunk_buf.send_buf);
    free(_read_recipe_buf);
    delete _reader_2_decoder_mq;
    return ;
}