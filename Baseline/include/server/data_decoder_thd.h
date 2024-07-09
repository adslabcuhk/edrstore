/**
 * @file data_decoder_thd.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interfaces of DataDecoderThd
 * @version 0.1
 * @date 2022-07-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef EDRSTORE_DATA_DECODE_THD
#define EDRSTORE_DATA_DECODE_THD

#include "client_var.h"
#include "../reduction/delta_comp.h"
#include "../compression/compress_util.h"
#include "../network/ssl_conn.h"
#include "../configure.h"

class DataDecoderThd {
    private:
        string my_name_ = "DataDecoderThd";

        // for config
        uint64_t send_chunk_batch_size_;
        uint64_t send_recipe_batch_size_;

        // for local compression
        CompressUtil* compress_util_;

        // for delta compression
        DeltaComp* delta_comp_;

        // SSL connection
        SSLConnection* server_channel_;

        /**
         * @brief decode a chunk
         * 
         * @param raw_chunk raw chunk
         * @param cur_client current client
         */
        void DecodeChunk(Reader2Decoder_t* raw_chunk, ClientVar* cur_client);

        /**
         * @brief send a batch of chunk
         * 
         * @param cur_client current client 
         */
        void SendChunks(ClientVar* cur_client);

        /**
         * @brief send the end flag 
         * 
         * @param cur_client current client
         * @param client_ip client ip
         */
        void SendEnd(ClientVar* cur_client, string& client_ip);

    public:
        /**
         * @brief Construct a new DataDecoderThd object
         * 
         * @param server_channel server channel
         */
        DataDecoderThd(SSLConnection* server_channel);

        /**
         * @brief Destroy the DataDecodeThd object
         * 
         */
        ~DataDecoderThd();

        /**
         * @brief the main process
         * 
         * @param cur_client current client
         */
        void Run(ClientVar* cur_client);
};

#endif