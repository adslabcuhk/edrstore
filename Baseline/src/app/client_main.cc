/**
 * @file client_main.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main process of client
 * @version 0.1
 * @date 2022-07-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/configure.h"

// for upload
#include "../../include/client/chunker_fp_thd.h"
#include "../../include/client/plain_similar_thd.h"
#include "../../include/client/key_gen_thd.h"
#include "../../include/client/select_comp_thd.h"
#include "../../include/client/sender_thd.h"

// for download
#include "../../include/client/download_writer_thd.h"
#include "../../include/client/data_retriever_thd.h"

#include <boost/thread/thread.hpp>

using namespace std;

struct timeval stime;
struct timeval etime;
double total_time = 0;

uint64_t total_data_size = 0;
uint64_t total_chunk_num = 0;
string client_stat_file_name = "client-stat";

Configure config("config.json");
string my_name = "ClientMain";
string log_file_name = "client-log";
ofstream log_file;
MQFactory<Chunk_t> chunk_mq_factory;
MQFactory<FeatureChunk_t> feature_chunk_mq_factory;
MQFactory<EncFeatureChunk_t> enc_feature_chunk_mq_factory;
MQFactory<SelectComp2Sender_t> select_comp_2_sender_mq_factory;
MQFactory<Retriever2Writer_t> retriever_2_writer_mq_factory;

uint32_t MQ_TYPE = LCK_FREE_MQ;

void Usage() {
    fprintf(stderr, "%s -t [u/d] -i [input file path] -m [key generation method].\n"
        "-t: operation ([u/d]):\n"
        "\tu: upload\n"
        "\td: download\n"
        "-m [key generation method]:\n"
        "\t0: Plain\n"
        "\t1: Server-aided MLE\n"
        "\t2: Encrypted Local Compression\n", my_name.c_str());
    return ;
}

int main(int argc, char* argv[]) {
    // for log file
    if (!tool::FileExist(log_file_name)) {
        // if the log file not exist, add the header
        log_file.open(log_file_name, ios_base::out);
        log_file << "total data size, " << "total chunk num, "
            << "input file, " << "opt, "
            << "logical data size, " << "logical chunk num, "
            << "client similar chunk num, " << "total time, "
            << "speed (MiB/s)" << endl;
    } else {
        // the log file exists
        log_file.open(log_file_name, ios_base::app | ios_base::out);
        ifstream client_stat_hdl;
        client_stat_hdl.open(client_stat_file_name, ios_base::in | ios_base::binary);
        client_stat_hdl.read((char*)&total_chunk_num, sizeof(uint64_t));
        client_stat_hdl.read((char*)&total_data_size, sizeof(uint64_t));
        client_stat_hdl.close();
    }

    // -------- main process --------

    const char opt_str[] = "t:i:m:";
    int option;

    if (argc < sizeof(opt_str)) {
        tool::Logging(my_name.c_str(), "wrong argc: %d\n", argc);
        Usage();
        exit(EXIT_FAILURE);
    }

    uint32_t opt_type;
    string input_file_path;
    uint32_t method_type;
    while ((option = getopt(argc, argv, opt_str)) != -1) {
        switch (option) {
            case 't': {
                if (strcmp("u", optarg) == 0) {
                    opt_type = UPLOAD_OPT;
                } else if (strcmp("d", optarg) == 0) {
                    opt_type = DOWNLOAD_OPT;
                } else {
                    tool::Logging(my_name.c_str(), "wrong operation type.\n");
                    Usage();
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 'i': {
                input_file_path.assign(optarg);
                break;
            }
            case 'm': {
                switch (atoi(optarg)) {
                    case PLAIN: {
                        method_type = PLAIN;
                        break;
                    }
                    case SERVER_AIDED_MLE: {
                        method_type = SERVER_AIDED_MLE;
                        break;
                    }
                    case ENC_COMP_MLE: {
                        method_type = ENC_COMP_MLE;
                        break;
                    }
                    default: {
                        tool::Logging(my_name.c_str(), "wrong key generation method type.\n");    
                        Usage();
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            }
            case '?': {
                tool::Logging(my_name.c_str(), "error optopt: %c\n", optopt);
                tool::Logging(my_name.c_str(), "error opterr: %d\n", opterr);
                Usage();
                exit(EXIT_FAILURE);
            }
        }
    }

    vector<boost::thread*> thd_list;
    boost::thread* tmp_thd;
    boost::thread::attributes thd_attrs;

    // for upload operation
    ChunkerFPThd* chunk_fp_thd = nullptr;
    PlainSimilarThd* plain_similar_thd = nullptr;
    KeyGenThd* key_gen_thd = nullptr;
    SelectCompThd* select_comp_thd = nullptr;
    SenderThd* sender_thd = nullptr;

    // for download operation
    DownloadWriterThd* download_writer_thd = nullptr;
    DataRetrieverThd* data_retriever_thd = nullptr;

    // for connection
    SSLConnection* server_channel = nullptr;
    pair<int, SSL*> server_conn_record;
    SSLConnection* km_channel = nullptr;
    pair<int, SSL*> km_conn_record;

    // connect to the cloud
    server_channel = new SSLConnection(config.GetStorageServerIP(),
        config.GetStorageServerPort(), IN_CLIENT_SIDE);
    server_conn_record = server_channel->ConnectSSL();

    // compute the file name hash
    uint32_t client_id = config.GetClientID();
    string full_name = input_file_path + to_string(client_id);
    uint8_t file_name_hash[CHUNK_HASH_SIZE] = {0};
    CryptoUtil* crypto_util = new CryptoUtil(CIPHER_TYPE, HASH_TYPE);
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    crypto_util->GenerateHash(md_ctx, (uint8_t*)&full_name[0], full_name.size(),
        file_name_hash);
    delete crypto_util;
    EVP_MD_CTX_free(md_ctx);

    switch (opt_type) {
        case UPLOAD_OPT: {
            ifstream input_file_hdl;
            tool::Logging(my_name.c_str(), "upload input file name: %s\n",
                input_file_path.c_str());
            input_file_hdl.open(input_file_path, ios_base::in | ios_base::binary);
            if (!input_file_hdl.is_open()) {
                tool::Logging(my_name.c_str(), "cannot open the input file: %s\n",
                    input_file_path.c_str());
                exit(EXIT_FAILURE);
            }

            chunk_fp_thd = new ChunkerFPThd();
            plain_similar_thd = new PlainSimilarThd();
            if (method_type == SERVER_AIDED_MLE || method_type == ENC_COMP_MLE) {
                km_channel = new SSLConnection(config.GetKeyServerIP(),
                    config.GetKeyServerPort(), IN_CLIENT_SIDE);
                km_conn_record = km_channel->ConnectSSL();
            } else {
                km_conn_record = {0, NULL};
            }
            key_gen_thd = new KeyGenThd(km_channel, km_conn_record, method_type);
            select_comp_thd = new SelectCompThd(method_type);
            sender_thd = new SenderThd(server_channel, server_conn_record,
                file_name_hash);

            AbsMQ<Chunk_t>* chunker_mq = chunk_mq_factory.CreateMQ(MQ_TYPE,
                CHUNK_QUEUE_SIZE);
            AbsMQ<FeatureChunk_t>* plain_similar_mq =
                feature_chunk_mq_factory.CreateMQ(MQ_TYPE, CHUNK_QUEUE_SIZE);
            AbsMQ<EncFeatureChunk_t>* key_gen_mq =
                enc_feature_chunk_mq_factory.CreateMQ(MQ_TYPE, CHUNK_QUEUE_SIZE);
            AbsMQ<SelectComp2Sender_t>* select_comp_mq =
                select_comp_2_sender_mq_factory.CreateMQ(MQ_TYPE, CHUNK_QUEUE_SIZE);
            
            // send the upload login to notify the server
            sender_thd->UploadLogin(file_name_hash);

            tmp_thd = new boost::thread(thd_attrs, boost::bind(&ChunkerFPThd::Run,
                chunk_fp_thd, &input_file_hdl, chunker_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&PlainSimilarThd::Run,
                plain_similar_thd, chunker_mq, plain_similar_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&KeyGenThd::Run,
                key_gen_thd, plain_similar_mq, key_gen_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&SelectCompThd::Run,
                select_comp_thd, key_gen_mq, select_comp_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&SenderThd::Run,
                sender_thd, select_comp_mq));
            thd_list.push_back(tmp_thd);

            gettimeofday(&stime, NULL);
            for (auto it : thd_list) {
                it->join();
            }
            gettimeofday(&etime, NULL);
            total_time += tool::GetTimeDiff(stime, etime);
            tool::Logging(my_name.c_str(), "%s finish.\n", input_file_path.c_str());

            for (auto it : thd_list) {
                delete it;
            }

            double speed = static_cast<double>(chunk_fp_thd->_total_file_size) /
                1024.0 / 1024.0 / total_time;
            total_chunk_num += chunk_fp_thd->_total_chunk_num;
            total_data_size += chunk_fp_thd->_total_file_size;
            log_file << total_data_size << ", "
                << total_chunk_num << ", "
                << input_file_path << ", upload, "
                << chunk_fp_thd->_total_file_size << ", "
                << chunk_fp_thd->_total_chunk_num << ", "
                << total_time << ", " << speed << endl;

            input_file_hdl.close();
            delete chunk_fp_thd;
            delete plain_similar_thd;
            delete key_gen_thd;
            delete select_comp_thd;
            delete sender_thd;

            // clear the MQ
            delete chunker_mq;
            delete plain_similar_mq;
            delete key_gen_mq;
            delete select_comp_mq;
            
            // clear the key server connection
            if (method_type == SERVER_AIDED_MLE || method_type == ENC_COMP_MLE) {
                delete km_channel;
            }

            ofstream client_stat_hdl;
            client_stat_hdl.open(client_stat_file_name, ios_base::trunc | ios_base::binary);
            client_stat_hdl.write((char*)&total_chunk_num, sizeof(uint64_t));
            client_stat_hdl.write((char*)&total_data_size, sizeof(uint64_t));
            client_stat_hdl.close();

            break;
        }
        case DOWNLOAD_OPT: {
            tool::Logging(my_name.c_str(), "download input file name: %s\n",
                input_file_path.c_str());
            
            data_retriever_thd = new DataRetrieverThd(server_channel,
                server_conn_record, file_name_hash, method_type);
            download_writer_thd = new DownloadWriterThd(input_file_path);

            AbsMQ<Retriever2Writer_t>* retriever_mq =
                retriever_2_writer_mq_factory.CreateMQ(MQ_TYPE, CHUNK_HASH_SIZE);

            data_retriever_thd->DownloadLogin(file_name_hash);

            tmp_thd = new boost::thread(thd_attrs, boost::bind(&DataRetrieverThd::Run,
                data_retriever_thd, retriever_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&DownloadWriterThd::Run,
                download_writer_thd, retriever_mq));
            thd_list.push_back(tmp_thd);

            gettimeofday(&stime, NULL);
            for (auto it : thd_list) {
                it->join();
            }
            gettimeofday(&etime, NULL);
            total_time += tool::GetTimeDiff(stime, etime);
            tool::Logging(my_name.c_str(), "%s finish.\n", input_file_path.c_str());

            for (auto it : thd_list) {
                delete it;
            }

            double speed = static_cast<double>(data_retriever_thd->_total_recv_data_size) /
                1024.0 / 1024.0 / total_time;
            log_file << total_data_size << ", "
                << total_chunk_num << ", "
                << input_file_path << ", download, "
                << data_retriever_thd->_total_recv_data_size << ", "
                << data_retriever_thd->_total_recv_chunk_num << ", "
                << total_time << ", " << speed << endl; 

            delete data_retriever_thd;
            delete download_writer_thd;

            // clear the MQ
            delete retriever_mq;
            break;
        }
    }
    
    // clear the connection
    delete server_channel;

    tool::Logging(my_name.c_str(), "total running time: %lf\n", total_time);
    log_file.close();
    tool::Logging(my_name.c_str(), "max mem usage: %lu KiB\n",
        tool::GetMaxMemoryUsage());
    return 0;
}