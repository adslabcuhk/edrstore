/**
 * @file ssl_conn.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief define the interface of ssl connection
 * @version 0.1
 * @date 2021-01-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef MY_CODEBASE_SSL_CONNECTION_H
#define MY_CODEBASE_SSL_CONNECTION_H

#include "../configure.h"

#include <netinet/in.h> // for sockaddr_in 
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

class SSLConnection {
    private:
        string my_name_ = "SSLConnection";
        // the socket address
        struct sockaddr_in socket_addr_;

        // server ip
        string server_ip_;

        // the port 
        int port_;

        // ssl context pointer
        SSL_CTX* ssl_ctx_ = NULL;

        // the listen file descriptor
        int listen_fd_;

    public:
        /**
         * @brief Construct a new SSLConnection object
         * 
         * @param ip the ip address
         * @param port the port number
         * @param type the type (client/server)
         */
        SSLConnection(string ip, int port, int type);

        /**
         * @brief Destroy the SSLConnection object
         * 
         */
        ~SSLConnection();

        /**
         * @brief finalize the connection
         * 
         * @param ssl_pair the pair of the server socket and ssl context
         */
        void Finish(pair<int, SSL*> ssl_pair);

        /**
         * @brief clear the corresponding accepted client socket and context
         * 
         * @param ssl_ptr the pointer to the SSL* of accepted client
         */
        void ClearAcceptedClientSd(SSL* ssl_ptr);

        /**
         * @brief connect to ssl
         * 
         * @return pair<int, SSL*> 
         */
        pair<int, SSL*> ConnectSSL();

        /**
         * @brief listen to a port 
         * 
         * @return pair<int, SSL*> 
         */
        pair<int, SSL*> ListenSSL();

        /**
         * @brief send the data to the given connection
         * 
         * @param ssl_conn the pointer to the connection
         * @param data the pointer to the data buffer
         * @param size the size of the input data
         * @return true success
         * @return false fail
         */
        bool SendData(SSL* ssl_conn, uint8_t* data, uint32_t size);

        /**
         * @brief receive the data from the given connection
         * 
         * @param connection the pointer to the connection
         * @param data the pointer to the data buffer
         * @param receiveDataSize the size of received data 
         * @return true success
         * @return false fail
         */
        bool ReceiveData(SSL* ssl_conn, uint8_t* data, uint32_t& recv_size);

        /**
         * @brief Get the Listen Fd object
         * 
         * @return int the listenFd
         */
        int GetListenFd() {
            return this->listen_fd_;
        }

        /**
         * @brief Get the Client Ip object
         * 
         * @param ip the ip of the client 
         * @param client_ssl the SSL connection of the client
         */
        void GetClientIp(string& ip, SSL* client_ssl) {
            int client_fd = SSL_get_fd(client_ssl);
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            getpeername(client_fd, (struct sockaddr*)&client_addr, &client_addr_len);
            ip.resize(INET_ADDRSTRLEN, 0);
            inet_ntop(AF_INET, &(client_addr.sin_addr), &ip[0], INET_ADDRSTRLEN);
            return ;
        }
};

#endif
