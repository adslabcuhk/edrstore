/**
 * @file client_main.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main program of client
 * @version 0.1
 * @date 2022-06-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/configure.h"

// for upload
#include "../../include/client/chunker_fp_thd.h"
#include "../../include/client/plain_similar_thd.h"
#include "../../include/client/key_gen_thd.h"
#include "../../include/client/cipher_similar_thd.h"
#include "../../include/client/select_comp_thd.h"
#include "../../include/client/sender_thd.h"

// for download
#include "../../include/client/download_writer_thd.h"
#include "../../include/client/data_retriever_thd.h"

#include <boost/thread/thread.hpp>

using namespace std;

//struct timeval ss;
//struct timeval ee;

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

#ifdef EDR_BREAKDOWN
string breakdown_file_name = "breakdown-client-file";
ofstream breakdown_client_file_hdl;
string breakdown_stat_file_name = "breakdown-stat";
#endif

void Usage() {
    fprintf(stderr, "%s -t [u/d] -i [input file path] -m .\n"
        "-t: operation ([u/d]):\n"
        "\tu: upload\n"
        "\td: download\n"
        "-m: method type:\n"
        "\t0: similar-aware encryption\n"
        "\t1: similar-aware encryption + local compression\n"
        "\t2: full EDR\n",
        my_name.c_str());
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
            << "speed (MiB/s), " << "upload data traffic, "
            << "version index size" << endl;
    } else {
        // the log file exists
        log_file.open(log_file_name, ios_base::app | ios_base::out);
        ifstream client_stat_hdl;
        client_stat_hdl.open(client_stat_file_name, ios_base::in | ios_base::binary);
        client_stat_hdl.read((char*)&total_chunk_num, sizeof(uint64_t));
        client_stat_hdl.read((char*)&total_data_size, sizeof(uint64_t));
        client_stat_hdl.close();
    }

#ifdef EDR_BREAKDOWN
    if (!tool::FileExist(breakdown_file_name)) {
        // if the breakdown file not exist, add the header
        breakdown_client_file_hdl.open(breakdown_file_name, ios_base::out);
        breakdown_client_file_hdl << "chunking size, " << "chunking time, "
            << "fp size, " << "fp time, "
            << "p_feature size, " << "p_feature time, "
            << "two_enc size, " << "two_enc time, " << "key gen time, "
            << "c_feature size, " << "c_feature time, "
            << "comp_pad size, " << "comp_pad time, "
            << "cache manage size, " << "cache manage time" << endl;
    } else {
        // the log file exists
        breakdown_client_file_hdl.open(breakdown_file_name, ios_base::app | ios_base::out);
    }
#endif

    // -------- main process --------

  //      gettimeofday(&ss, NULL);
    // printf("%d", ee.tv_sec);
    // printf("%d", ee.tv_usec);
    //cout<<ss.tv_sec<<" "<<ss.tv_usec<<endl;

    const char opt_str[] = "t:i:m:";
    int option;

    if (argc < (int)sizeof(opt_str)) {
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
                    case ONLY_SIMILAR_ENC: {
                        method_type = ONLY_SIMILAR_ENC;
                        break;
                    }
                    case SIMILAR_ENC_COMP: {
                        method_type = SIMILAR_ENC_COMP;
                        break;
                    }
                    case FULL_EDR: {
                        method_type = FULL_EDR;
                        break;
                    }
                    default: {
                        tool::Logging(my_name.c_str(), "wrong EDRStore design type.\n");
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
    CipherSimilarThd* cipher_similar_thd = nullptr;
    SelectCompThd* select_comp_thd = nullptr;
    SenderThd* sender_thd = nullptr;
    CacheMeta* cache_meta = nullptr;

    // for download operation
    DownloadWriterThd* download_writer_thd = nullptr;
    DataRetrieverThd* data_retriever_thd = nullptr;

    // for connection 
    SSLConnection* server_channel;
    pair<int, SSL*> server_conn_record;
    SSLConnection* km_channel = nullptr;
    pair<int, SSL*> km_conn_record;

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
            km_channel = new SSLConnection(config.GetKeyServerIP(),
                config.GetKeyServerPort(), IN_CLIENT_SIDE);
            km_conn_record = km_channel->ConnectSSL();
            key_gen_thd = new KeyGenThd(km_channel, km_conn_record);
            cipher_similar_thd = new CipherSimilarThd();
            cache_meta = new CacheMeta(server_channel, server_conn_record);
            select_comp_thd = new SelectCompThd(cache_meta, method_type);
            sender_thd = new SenderThd(server_channel, server_conn_record,
                file_name_hash, cache_meta);

#ifdef EDR_BREAKDOWN
            // restore the breakdown status
            ifstream in_breakdown_stat_hdl;
            // chunking + fp thread
            in_breakdown_stat_hdl.open(breakdown_stat_file_name, ios_base::in);
            in_breakdown_stat_hdl.read((char*)&chunk_fp_thd->_total_chunking_data_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&chunk_fp_thd->_total_chunking_time,
                sizeof(double));
            in_breakdown_stat_hdl.read((char*)&chunk_fp_thd->_total_fp_data_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&chunk_fp_thd->_total_fp_time,
                sizeof(double));

            // plain similar thread
            in_breakdown_stat_hdl.read((char*)&plain_similar_thd->_total_plain_feature_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&plain_similar_thd->_total_plain_feature_time,
                sizeof(double));

            // key gen thread
            in_breakdown_stat_hdl.read((char*)&key_gen_thd->_total_two_enc_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&key_gen_thd->_total_two_enc_time,
                sizeof(double));
            in_breakdown_stat_hdl.read((char*)&key_gen_thd->_total_key_gen_time,
                sizeof(double));

            // cipher feature thread
            in_breakdown_stat_hdl.read((char*)&cipher_similar_thd->_total_cipher_feature_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&cipher_similar_thd->_total_cipher_feature_time,
                sizeof(double));

            // select comp thread
            in_breakdown_stat_hdl.read((char*)&select_comp_thd->_total_comp_pad_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&select_comp_thd->_total_comp_pad_time,
                sizeof(double));
            in_breakdown_stat_hdl.read((char*)&select_comp_thd->_total_cache_manage_size,
                sizeof(uint64_t));
            in_breakdown_stat_hdl.read((char*)&select_comp_thd->_total_cache_manage_time,
                sizeof(double));

            in_breakdown_stat_hdl.close();
#endif

            
            AbsMQ<Chunk_t>* chunker_mq = chunk_mq_factory.CreateMQ(MQ_TYPE,
                CHUNK_QUEUE_SIZE);
            AbsMQ<FeatureChunk_t>* plain_similar_mq =
                feature_chunk_mq_factory.CreateMQ(MQ_TYPE, CHUNK_QUEUE_SIZE);
            AbsMQ<EncFeatureChunk_t>* key_gen_mq =
                enc_feature_chunk_mq_factory.CreateMQ(MQ_TYPE, CHUNK_QUEUE_SIZE);
            AbsMQ<EncFeatureChunk_t>* cipher_similar_mq =
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
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&CipherSimilarThd::Run, 
                cipher_similar_thd, key_gen_mq, cipher_similar_mq));
            thd_list.push_back(tmp_thd);
            tmp_thd = new boost::thread(thd_attrs, boost::bind(&SelectCompThd::Run,
                select_comp_thd, cipher_similar_mq, select_comp_mq));
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
                << cache_meta->_total_similar_chunk << ", "
                << total_time << ", " << speed << ", "
                << sender_thd->_total_send_data_size << ", "
                << sender_thd->_cur_version_idx_size << endl;
            
#ifdef EDR_BREAKDOWN
            breakdown_client_file_hdl << chunk_fp_thd->_total_chunking_data_size << ", "
                << chunk_fp_thd->_total_chunking_time << ", "
                << chunk_fp_thd->_total_fp_data_size << ", "
                << chunk_fp_thd->_total_fp_time << ", "
                << plain_similar_thd->_total_plain_feature_size << ", "
                << plain_similar_thd->_total_plain_feature_time << ", "
                << key_gen_thd->_total_two_enc_size << ", "
                << key_gen_thd->_total_two_enc_time << ", "
                << key_gen_thd->_total_key_gen_time << ", "
                << cipher_similar_thd->_total_cipher_feature_size << ", "
                << cipher_similar_thd->_total_cipher_feature_time << ", "
                << select_comp_thd->_total_comp_pad_size << ", "
                << select_comp_thd->_total_comp_pad_time << ", "
                << select_comp_thd->_total_cache_manage_size << ", "
                << select_comp_thd->_total_cache_manage_time << endl;
            
            // store the breakdown stat
            ofstream out_breakdown_stat_hdl;
            out_breakdown_stat_hdl.open(breakdown_stat_file_name,
                ios_base::trunc | ios_base::binary);
            // chunking + fp thread
            out_breakdown_stat_hdl.write((char*)&chunk_fp_thd->_total_chunking_data_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&chunk_fp_thd->_total_chunking_time,
                sizeof(double));
            out_breakdown_stat_hdl.write((char*)&chunk_fp_thd->_total_fp_data_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&chunk_fp_thd->_total_fp_time,
                sizeof(double));
            
            // plain similar thread
            out_breakdown_stat_hdl.write((char*)&plain_similar_thd->_total_plain_feature_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&plain_similar_thd->_total_plain_feature_time,
                sizeof(double));
            
            // key gen thread
            out_breakdown_stat_hdl.write((char*)&key_gen_thd->_total_two_enc_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&key_gen_thd->_total_two_enc_time,
                sizeof(double));
            out_breakdown_stat_hdl.write((char*)&key_gen_thd->_total_key_gen_time,
                sizeof(double));

            // cipher similar thread
            out_breakdown_stat_hdl.write((char*)&cipher_similar_thd->_total_cipher_feature_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&cipher_similar_thd->_total_cipher_feature_time,
                sizeof(double));

            // select comp thread
            out_breakdown_stat_hdl.write((char*)&select_comp_thd->_total_comp_pad_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&select_comp_thd->_total_comp_pad_time,
                sizeof(double));
            out_breakdown_stat_hdl.write((char*)&select_comp_thd->_total_cache_manage_size,
                sizeof(uint64_t));
            out_breakdown_stat_hdl.write((char*)&select_comp_thd->_total_cache_manage_time,
                sizeof(double));

            out_breakdown_stat_hdl.close();
#endif

            input_file_hdl.close();
            delete chunk_fp_thd;
            delete plain_similar_thd;
            delete key_gen_thd;
            delete cipher_similar_thd;
            delete select_comp_thd;
            delete sender_thd;
            delete km_channel;
            delete cache_meta;

            // clear the MQ
            delete chunker_mq;
            delete plain_similar_mq;
            delete cipher_similar_mq;
            delete key_gen_mq; 
            delete select_comp_mq;

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
                << "0, " << total_time << ", " << speed << endl; 

            delete data_retriever_thd;
            delete download_writer_thd;

            // clear the MQ
            delete retriever_mq;
            break;
        }
    }

    // clear the connection
    delete server_channel;

#ifdef EDR_BREAKDOWN
    breakdown_client_file_hdl.close();
#endif

    tool::Logging(my_name.c_str(), "total running time: %lf\n", total_time);    
    log_file.close();
    tool::Logging(my_name.c_str(), "max mem usage: %lu KiB\n",
        tool::GetMaxMemoryUsage());
    
    // cout<< std::put_time(std::localtime(&t), "%F %T ")
    // std::cout << system_clock::now() << '\n';
//    gettimeofday(&ee, NULL);
    // printf("%d", ee.tv_sec);
    // printf("%d", ee.tv_usec);
  //  cout<<ee.tv_sec<<" "<<ee.tv_usec<<endl;
    

    return 0;
}
