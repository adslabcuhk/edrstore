/**
 * @file server_main.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief the main process of the storage server
 * @version 0.1
 * @date 2022-07-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "../../include/configure.h"
#include "../../include/network/ssl_conn.h"
#include "../../include/database/db_factory.h"
#include "../../include/server/server_opt_thd.h"

// for the interrupt
#include <signal.h>
#include <boost/thread/thread.hpp>

using namespace std;

Configure config("config.json");

SSLConnection* server_channel;
DatabaseFactory db_factory;
AbsDatabase* fp_2_addr_db;
AbsDatabase* feature_2_fp_db;
vector<boost::thread*> thd_list;

// the server main thread
ServerOptThd* server_opt_thd;

string my_name = "EDRServer";

void Usage() {
    fprintf(stderr, "Usage: %s\n", 
        my_name.c_str());
    return ;
}

void CTRLC(int s) {
    tool::Logging(my_name.c_str(), "terminated with ctrl+c interrupt.\n");

    // -------- clean up --------
    for (auto it : thd_list) {
        it->join();
    }
    
    for (auto it : thd_list) {
        delete it;
    }
    delete server_opt_thd;
    tool::Logging(my_name.c_str(), "clear all server threads.\n");

    delete fp_2_addr_db;
    delete feature_2_fp_db;
    delete server_channel;

    tool::Logging(my_name.c_str(), "clear all DBs and network connection.\n");
    cout << "Max mem usage: " << tool::GetMaxMemoryUsage() << "KiB" << endl;
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {

    // -------- main process --------

    struct sigaction sig_hdl;
    sig_hdl.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig_hdl, 0);

    sig_hdl.sa_handler = CTRLC;
    sigaction(SIGKILL, &sig_hdl, 0);
    sigaction(SIGINT, &sig_hdl, 0);

    srand(tool::GetStrongSeed());

    boost::thread* tmp_thd;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);

    fp_2_addr_db = db_factory.CreateDatabase(IN_MEMORY_DB,
        config.GetFp2ChunkDBName());
    feature_2_fp_db = db_factory.CreateDatabase(IN_MEMORY_DB,
        config.GetFeature2FpDBName());

    server_channel = new SSLConnection(config.GetStorageServerIP(),
        config.GetStorageServerPort(), IN_SERVER_SIDE);

    // init
    server_opt_thd = new ServerOptThd(server_channel, fp_2_addr_db,
        feature_2_fp_db);

    /**
     * |---------------------------------------|
     * |Finish the initialization of the server|
     * |---------------------------------------|
     */

    while (true) {
        tool::Logging(my_name.c_str(), "waiting the req from the client.\n");
        SSL* client_ssl = server_channel->ListenSSL().second;
        tmp_thd = new boost::thread(attrs, boost::bind(&ServerOptThd::Run,
            server_opt_thd, client_ssl));
        thd_list.push_back(tmp_thd);
    } 

    return 0;
}