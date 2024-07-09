/**
 * @file server_opt_thd.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interfaces fo ServerOptThd 
 * @version 0.1
 * @date 2022-07-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/server/server_opt_thd.h"

/**
 * @brief Construct a new ServerOptThd object
 * 
 * @param server_channel the server channel
 * @param fp_2_addr_db the fp to address index
 * @param feature_2_fp_db the feature to base hash index
 */
ServerOptThd::ServerOptThd(SSLConnection* server_channel, 
    AbsDatabase* fp_2_addr_db, AbsDatabase* feature_2_fp_db) {
    server_channel_ = server_channel;
    fp_2_addr_db_ = fp_2_addr_db;
    feature_2_fp_db_ = feature_2_fp_db;

    // init the upload
    storage_core_ = new StorageCore();
    
    data_recv_thd_ = new DataRecvThd(server_channel_, fp_2_addr_db_);
    data_writer_thd_ = new DataWriterThd(fp_2_addr_db_, feature_2_fp_db_,
        storage_core_);
    
    data_reader_thd_ = new DataReaderThd(fp_2_addr_db_, storage_core_);
    data_decode_thd_ = new DataDecoderThd(server_channel_);
    
    this->LoadStat();
}

/**
 * @brief Destroy the ServerOptThd object
 * 
 */
ServerOptThd::~ServerOptThd() {
    this->StoreStat();
    delete storage_core_;

    delete data_recv_thd_;
    delete data_writer_thd_;

    delete data_reader_thd_;
    delete data_decode_thd_;

    for (auto it : client_lck_idx_) {
        delete it.second;
    }
    fprintf(stderr, "========ServerOptThd Info========\n");
    fprintf(stderr, "total recv upload req: %lu\n", 
        _total_upload_opt_num);
    fprintf(stderr, "total recv download req: %lu\n", 
        _total_download_opt_num);
    fprintf(stderr, "=================================\n");
}

/**
 * @brief the main thread
 * 
 * @param client_ssl 
 */
void ServerOptThd::Run(SSL* client_ssl) {
    boost::thread* tmp_thd;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    vector<boost::thread*> thd_list;

    SendMsgBuffer_t recv_buf;
    recv_buf.send_buf = (uint8_t*) malloc(sizeof(NetworkHead_t) + CHUNK_HASH_SIZE);
    recv_buf.header = (NetworkHead_t*) recv_buf.send_buf;
    recv_buf.header->size = 0;
    recv_buf.data_buf = recv_buf.send_buf + sizeof(NetworkHead_t);
    uint32_t recv_size = 0;

    tool::Logging(my_name_.c_str(), "the main thread is running.\n");

    // start to receive the login data type
    if (!server_channel_->ReceiveData(client_ssl, recv_buf.send_buf,
        recv_size)) {
        tool::Logging(my_name_.c_str(), "recv the login message error.\n");
        exit(EXIT_FAILURE);
    }

    // add a client lck here (ensure only one client with the same id)
    uint32_t client_id = recv_buf.header->client_id;
    this->LockClientID(client_id);
    
    // -------- main process --------
    int opt_type = 0;
    switch (recv_buf.header->msg_type) {
        case CLIENT_LOGIN_UPLOAD: {
            opt_type = UPLOAD_OPT;
            break;
        }
        case CLIENT_LOGIN_DOWNLOAD: {
            opt_type = DOWNLOAD_OPT;
            break;
        }
        default: {
            tool::Logging(my_name_.c_str(), "wrong client login type.\n");
            exit(EXIT_FAILURE);
        }
    }

    // check the file status
    // convert the file name hash to the file path
    char file_hash_buf[CHUNK_HASH_SIZE * 2 + 1];
    for (size_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(file_hash_buf + i * 2, "%02x", recv_buf.data_buf[i]);
    }
    string file_name;
    file_name.assign(file_hash_buf, CHUNK_HASH_SIZE * 2);
    string recipe_path = config.GetRecipeRootPath() + file_name +
        config.GetRecipeSuffix();
    if (!this->CheckFileStat(recipe_path, opt_type)) {
        recv_buf.header->msg_type = SERVER_FILE_NON_EXIST;
        if (!server_channel_->SendData(client_ssl, recv_buf.send_buf,
            sizeof(NetworkHead_t))) {
            tool::Logging(my_name_.c_str(), "send the file reply error.\n");
            exit(EXIT_FAILURE);
        }

        // wait the client to close the connection
        if (!server_channel_->ReceiveData(client_ssl, recv_buf.send_buf,
            recv_size)) {
            tool::Logging(my_name_.c_str(), "client close the socket connection.\n");
            server_channel_->ClearAcceptedClientSd(client_ssl);
        } else {
            tool::Logging(my_name_.c_str(), "client does not close the connection.\n");
            exit(EXIT_FAILURE);
        }

        // clear the tmp variable
        free(recv_buf.send_buf);
        this->UnlockClientID(client_id);
        return ;
    } else {
        tool::Logging(my_name_.c_str(), "file status check successfully.\n");
    }
    // check done

    // init the vars for this client
    ClientVar* cur_client;
    switch (opt_type) {
        case UPLOAD_OPT: {
            // update the req number
            _total_upload_opt_num++;
            tool::Logging(my_name_.c_str(), "recv the upload req from client: %u\n",
                client_id);
            cur_client = new ClientVar(client_id, client_ssl, UPLOAD_OPT,
                recipe_path);
            
            tmp_thd = new boost::thread(attrs, boost::bind(&DataRecvThd::Run,
                data_recv_thd_, cur_client));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(attrs, boost::bind(&DataWriterThd::Run,
                data_writer_thd_, cur_client));
            thd_list.push_back(tmp_thd);

            // send the upload-response to the client
            recv_buf.header->msg_type = SERVER_LOGIN_RESPONSE;
            if (!server_channel_->SendData(client_ssl, recv_buf.send_buf,
                sizeof(NetworkHead_t))) {
                tool::Logging(my_name_.c_str(), "send the upload-login response error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case DOWNLOAD_OPT: {
            _total_download_opt_num++;
            tool::Logging(my_name_.c_str(), "recv the download req from client: %u\n",
                client_id);
            cur_client = new ClientVar(client_id, client_ssl, DOWNLOAD_OPT,
                recipe_path);
            
            // send the download-response to the client (include the file recipe header)
            recv_buf.header->msg_type = SERVER_LOGIN_RESPONSE;
            cur_client->_recipe_read_hdl.read((char*)recv_buf.data_buf,
                sizeof(FileRecipeHead_t));
            if (!server_channel_->SendData(client_ssl, recv_buf.send_buf,
                sizeof(NetworkHead_t) + sizeof(FileRecipeHead_t))) {
                tool::Logging(my_name_.c_str(), "send the download-login response error.\n");
                exit(EXIT_FAILURE);
            }

            tmp_thd = new boost::thread(attrs, boost::bind(&DataReaderThd::Run,
                data_reader_thd_, cur_client));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(attrs, boost::bind(&DataDecoderThd::Run,
                data_decode_thd_, cur_client));
            thd_list.push_back(tmp_thd);

            break;
        }
    }

    double total_time = 0;
    struct timeval stime;
    struct timeval etime;
    gettimeofday(&stime, NULL);
    for (auto it : thd_list) {
        it->join();
    }
    gettimeofday(&etime, NULL);
    total_time += tool::GetTimeDiff(stime, etime);

    if (opt_type == UPLOAD_OPT) {
        this->PrintClientLog();
    }

    // clean up
    for (auto it : thd_list) {
        delete it;
    }
    thd_list.clear();

    // clean up client variables
    delete cur_client;
    free(recv_buf.send_buf);
    this->UnlockClientID(client_id);

    tool::Logging(my_name_.c_str(), "total running time of client %u: %lf\n",
        client_id, total_time); 

    return ;
}

/**
 * @brief lock the mutex of a given client id
 * 
 * @param client_id the client id
 */
void ServerOptThd::LockClientID(uint32_t client_id) {
    boost::mutex* tmp_lck;
    {
        lock_guard<mutex> lck(client_idx_lck_);
        auto client_lck_ret = client_lck_idx_.find(client_id);
        if (client_lck_ret != client_lck_idx_.end()) {
            // try to acquire the lck
            tmp_lck = client_lck_idx_[client_id];
        } else {
            // add a new lck to the current index
            tmp_lck = new boost::mutex();
            client_lck_idx_[client_id] = tmp_lck;
        }
    }
    tmp_lck->lock();
    return ;
}

/**
 * @brief unlock the mutex of a given client id
 * 
 * @param client_id the client id
 */
void ServerOptThd::UnlockClientID(uint32_t client_id) {
    boost::mutex* tmp_lck;
    {
        lock_guard<mutex> lck(client_idx_lck_);
        tmp_lck = client_lck_idx_[client_id];
    }
    tmp_lck->unlock();
    return ;
}

/**
 * @brief check the file status
 * 
 * @param recipe_path the full recipe path
 * @param opt_type the opt type
 * @return true success
 * @return false fail
 */
bool ServerOptThd::CheckFileStat(string& recipe_path, int opt_type) {
    if (tool::FileExist(recipe_path)) {
        // the file exists
        switch (opt_type) {
            case UPLOAD_OPT: {
                tool::Logging(my_name_.c_str(), "%s exists, overwrite it.\n",
                    recipe_path.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(my_name_.c_str(), "%s exists, access it.\n",
                    recipe_path.c_str());
                break;
            }
        }
    } else {
        switch (opt_type) {
            case UPLOAD_OPT: {
                tool::Logging(my_name_.c_str(), "%s not exists, create it.\n",
                    recipe_path.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(my_name_.c_str(), "%s not exists, restore reject.\n",
                    recipe_path.c_str());
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief load previous stat 
 * 
 */
void ServerOptThd::LoadStat() {
    if (tool::FileExist(persist_stat_name_)) {
        // the stat file exists
        ifstream prev_stat_file_hdl;
        prev_stat_file_hdl.open(persist_stat_name_, ios_base::binary |
            ios_base::in);
        if (!prev_stat_file_hdl.is_open()) {
            tool::Logging(my_name_.c_str(), "cannot open the stat file.\n");
            exit(EXIT_FAILURE);
        } else {
            // for dedup
            prev_stat_file_hdl.read(
                (char*)&data_recv_thd_->_total_logical_chunk_num,
                sizeof(uint64_t));
            prev_stat_file_hdl.read(
                (char*)&data_recv_thd_->_total_logical_data_size,
                sizeof(uint64_t));
            prev_stat_file_hdl.read(
                (char*)&data_recv_thd_->_total_unique_chunk_num,
                sizeof(uint64_t));
            prev_stat_file_hdl.read(
                (char*)&data_recv_thd_->_total_unique_data_size,
                sizeof(uint64_t));

            // for delta compression
            prev_stat_file_hdl.read(
                (char*)&data_writer_thd_->_total_similar_chunk_num,
                sizeof(uint64_t));
            prev_stat_file_hdl.read(
                (char*)&data_writer_thd_->_total_similar_data_size,
                sizeof(uint64_t));
            prev_stat_file_hdl.read(
                (char*)&data_writer_thd_->_total_delta_size,
                sizeof(uint64_t));

            // for total stored data
            prev_stat_file_hdl.read((char*)&storage_core_->_write_chunk_num,
                sizeof(uint64_t));
            prev_stat_file_hdl.read((char*)&storage_core_->_write_data_size,
                sizeof(uint64_t));

            prev_stat_file_hdl.close();
        }
    }

    return ;
}

/**
 * @brief store current stat
 * 
 */
void ServerOptThd::StoreStat() {
    ofstream prev_stat_file_hdl;
    prev_stat_file_hdl.open(persist_stat_name_, ios_base::trunc | ios_base::binary);
    if (!prev_stat_file_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot open the stat file.\n");
        exit(EXIT_FAILURE);
    } else {
        // for dedup
        prev_stat_file_hdl.write(
            (char*)&data_recv_thd_->_total_logical_chunk_num,
            sizeof(uint64_t));
        prev_stat_file_hdl.write(
            (char*)&data_recv_thd_->_total_logical_data_size,
            sizeof(uint64_t));
        prev_stat_file_hdl.write(
            (char*)&data_recv_thd_->_total_unique_chunk_num,
            sizeof(uint64_t));
        prev_stat_file_hdl.write(
            (char*)&data_recv_thd_->_total_unique_data_size,
            sizeof(uint64_t));

        // for delta compression
        prev_stat_file_hdl.write(
            (char*)&data_writer_thd_->_total_similar_chunk_num,
            sizeof(uint64_t));
        prev_stat_file_hdl.write(
            (char*)&data_writer_thd_->_total_similar_data_size,
            sizeof(uint64_t));
        prev_stat_file_hdl.write(
            (char*)&data_writer_thd_->_total_delta_size,
            sizeof(uint64_t));

        // for total stored data
        prev_stat_file_hdl.write((char*)&storage_core_->_write_chunk_num,
            sizeof(uint64_t));
        prev_stat_file_hdl.write((char*)&storage_core_->_write_data_size,
            sizeof(uint64_t));
        
        prev_stat_file_hdl.close();
    }

    return ;
}

/**
 * @brief print the info of curClient
 * 
 */
void ServerOptThd::PrintClientLog() {
    ofstream server_log_hdl;
    bool is_first_server_log = !(tool::FileExist(server_log_name_));
    server_log_hdl.open(server_log_name_, ios_base::app | ios_base::out);
    if (!server_log_hdl.is_open()) {
        tool::Logging(my_name_.c_str(), "cannot open the server log file.\n");
        exit(EXIT_FAILURE);
    } else {
        if (is_first_server_log) {
            server_log_hdl << "logical size, " << "logical chunk num, "
                << "unique size, " <<  "unique chunk num, "
                << "similar size, " << "similar chunk num, "
                << "delta size, " << "storage size" << endl;
        }
        server_log_hdl << data_recv_thd_->_total_logical_data_size << ", " 
            << data_recv_thd_->_total_logical_chunk_num << ", "
            << data_recv_thd_->_total_unique_data_size << ", "
            << data_recv_thd_->_total_unique_chunk_num << ", "
            << data_writer_thd_->_total_similar_data_size << ", " 
            << data_writer_thd_->_total_similar_chunk_num << ", " 
            << data_writer_thd_->_total_delta_size << ", " 
            << storage_core_->_write_data_size << endl;
        server_log_hdl.close();
    }
}