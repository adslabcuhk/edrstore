/**
 * @file ssl_connection.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the interface of SSLConnection
 * @version 0.1
 * @date 2021-01-20
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/network/ssl_conn.h"

/**
 * @brief Construct a new SSLConnection object
 * 
 * @param ip the ip address
 * @param port the port number
 * @param type the type (client/server)
 */
SSLConnection::SSLConnection(string ip, int port, int type) {
    server_ip_ = ip;
    port_ = port;
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    
    // init the SSL lib
    OPENSSL_init_ssl(0, NULL);
    memset(&socket_addr_, 0, sizeof(socket_addr_));
    
    socket_addr_.sin_port = htons(port_);
    socket_addr_.sin_family = AF_INET;

    // load the cert and key
    string key_file_str;
    string crt_file_str;
    string ca_file_str;

    ca_file_str.assign(CA_CERT);
    int enable = 1;
    switch (type) {
        case IN_SERVER_SIDE:
            ssl_ctx_ = SSL_CTX_new(TLS_server_method());
            // need to reconsider this option when using epoll
            SSL_CTX_set_mode(ssl_ctx_, SSL_MODE_AUTO_RETRY); // handle for multiple time hand shakes
            key_file_str.assign(SERVER_KEY);
            crt_file_str.assign(SERVER_CERT);
            socket_addr_.sin_addr.s_addr = htons(INADDR_ANY);
            if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
                tool::Logging(my_name_.c_str(), "cannot set the port reusable.\n");
                exit(EXIT_FAILURE);
            }
            if (bind(listen_fd_, (struct sockaddr*)&socket_addr_, sizeof(socket_addr_)) == -1) {
                tool::Logging(my_name_.c_str(), "cannot not bind to socketFd\n"
                                "\tMay cause by shutdown server before client\n"
                                "\tWait for 1 min and try again\n");
                tool::Logging(my_name_.c_str(), "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (listen(listen_fd_, 10) == -1) {
                tool::Logging(my_name_.c_str(), "cannot listen this socket.\n");
                tool::Logging(my_name_.c_str(), "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;
        case IN_CLIENT_SIDE:
            ssl_ctx_ = SSL_CTX_new(TLS_client_method());
            key_file_str.assign(CLIENT_KEY);
            crt_file_str.assign(CLIENT_CERT);
            socket_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
            break;
        default:
            tool::Logging(my_name_.c_str(), "error connection type.\n");
            exit(EXIT_FAILURE);
    }

    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, NULL);
    if (!SSL_CTX_load_verify_locations(ssl_ctx_, ca_file_str.c_str(), NULL)) {
        tool::Logging(my_name_.c_str(), "load ca crt error\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_use_certificate_file(ssl_ctx_, crt_file_str.c_str(), SSL_FILETYPE_PEM)) {
        tool::Logging(my_name_.c_str(), "load cert error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_use_PrivateKey_file(ssl_ctx_, key_file_str.c_str(), SSL_FILETYPE_PEM)) {
        tool::Logging(my_name_.c_str(), "load private key error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (!SSL_CTX_check_private_key(ssl_ctx_)) {
        tool::Logging(my_name_.c_str(), "check private key error.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    switch (type) {
        case IN_SERVER_SIDE:
            tool::Logging(my_name_.c_str(), "init the connection to port %d\n", port_);
            break;
        case IN_CLIENT_SIDE:
            tool::Logging(my_name_.c_str(), "init the connection to <%s:%d>\n", server_ip_.c_str(), port_);
            break; 
        default:
            tool::Logging(my_name_.c_str(), "error connection type.\n");
            exit(EXIT_FAILURE);
    }
}


/**
 * @brief Destroy the SSLConnection object
 * 
 */
SSLConnection::~SSLConnection() {
    SSL_CTX_free(ssl_ctx_);
    close(listen_fd_);
}

/**
 * @brief finalize the connection
 * 
 * @param ssl_pair the pair of the server socket and ssl context
 */
void SSLConnection::Finish(pair<int, SSL*> ssl_pair) {
    int ret = SSL_shutdown(ssl_pair.second);
    if (ret != 0) {
        tool::Logging(my_name_.c_str(), "first shutdown the socket in client side error, "
                "ret: %d\n", ret);
        exit(EXIT_FAILURE);
    }

    // check the ssl shutdown flag state
    if ((SSL_get_shutdown(ssl_pair.second) & SSL_SENT_SHUTDOWN) != 1) {
        tool::Logging(my_name_.c_str(), "set the sent shutdown flag error.\n");
        exit(EXIT_FAILURE);
    }

    // wait the close alert from another peer
    int tmp;
    int ret_stat;
    ret_stat = SSL_read(ssl_pair.second, (uint8_t*)&tmp, sizeof(tmp));
    if (SSL_get_error(ssl_pair.second, ret_stat) != SSL_ERROR_ZERO_RETURN) {
        tool::Logging(my_name_.c_str(), "receive shutdown flag error.\n");
        exit(EXIT_FAILURE);
    }
    tmp = SSL_shutdown(ssl_pair.second);
    if (tmp != 1) {
        tool::Logging(my_name_.c_str(), "shutdown the ssl socket fail, ret: %d\n", tmp);
        exit(EXIT_FAILURE);
    }

    tool::Logging(my_name_.c_str(), "shutdown the SSL connection successfully.\n");

    SSL_free(ssl_pair.second);
    close(ssl_pair.first);
    return ;
}

/**
 * @brief clear the corresponding accepted client socket and context
 * 
 * @param ssl_ptr the pointer to the SSL* of accepted client
 */
void SSLConnection::ClearAcceptedClientSd(SSL* ssl_ptr) {
    int sd = SSL_get_fd(ssl_ptr);
    SSL_free(ssl_ptr);
    close(sd);
    return ;
}

/**
 * @brief connect to ssl
 * 
 * @return pair<int, SSL*> 
 */
pair<int, SSL*> SSLConnection::ConnectSSL() {
    int socket_fd;
    SSL* ssl_ptr;
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(socket_fd, (struct sockaddr*)&socket_addr_, sizeof(socket_addr_)) < 0) {
        tool::Logging(my_name_.c_str(), "cannot connect on the socket: %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    ssl_ptr = SSL_new(ssl_ctx_);
    if (!SSL_set_fd(ssl_ptr, socket_fd)) {
        tool::Logging(my_name_.c_str(), "cannot combine the fd and ssl.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    
    // start the SSL handshake 
    if (SSL_connect(ssl_ptr) != 1) {
        tool::Logging(my_name_.c_str(), "ssl connect fails.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    } 

    return make_pair(socket_fd, ssl_ptr);
}

/**
 * @brief listen to a port 
 * 
 * @return pair<int, SSL*> 
 */
pair<int, SSL*> SSLConnection::ListenSSL() {
    int socket_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    socket_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_addr_len); 

    if (socket_fd < 0) {
        tool::Logging(my_name_.c_str(), "socket listen fails: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    SSL* ssl_ptr = SSL_new(ssl_ctx_);
    if (!SSL_set_fd(ssl_ptr, socket_fd)) {
        tool::Logging(my_name_.c_str(), "cannot combine the fd and ssl.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_accept(ssl_ptr) != 1) {
        tool::Logging(my_name_.c_str(), "accept the connection fails.\n");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return make_pair(socket_fd, ssl_ptr);
}

/**
 * @brief send the data to the given connection
 * 
 * @param ssl_conn the pointer to the connection
 * @param data the pointer to the data buffer
 * @param size the size of the input data
 * @return true success
 * @return false fail
 */
bool SSLConnection::SendData(SSL* ssl_conn, uint8_t* data, uint32_t size) {
    int write_stat;
    write_stat = SSL_write(ssl_conn, (char*)&size, sizeof(uint32_t));
    if (write_stat <= 0) {
        tool::Logging(my_name_.c_str(), "write the data fails. ret: %d\n",
            SSL_get_error(ssl_conn, write_stat));
        ERR_print_errors_fp(stderr);
        return false;
    }

    uint32_t current_send_size = 0;
    while (current_send_size < size) {
        write_stat = SSL_write(ssl_conn, data + current_send_size,
            size - current_send_size);
        if (write_stat <= 0) {
            tool::Logging(my_name_.c_str(), "write the data fails. ret: %d\n",
                SSL_get_error(ssl_conn, write_stat));
            ERR_print_errors_fp(stderr);
            return false;
        }
        current_send_size += write_stat;
    }

    return true;
}

/**
 * @brief receive the data from the given connection
 * 
 * @param connection the pointer to the connection
 * @param data the pointer to the data buffer
 * @param receiveDataSize the size of received data 
 * @return true success
 * @return false fail
 */
bool SSLConnection::ReceiveData(SSL* ssl_conn, uint8_t* data, uint32_t& recv_size) {
    uint32_t msg_size = 0;
    int read_stat;
    read_stat = SSL_read(ssl_conn, (char*)&msg_size, sizeof(uint32_t));
    if (read_stat <= 0) {
        if (SSL_get_error(ssl_conn, read_stat) == SSL_ERROR_ZERO_RETURN) {
            tool::Logging(my_name_.c_str(), "TLS/SSL peer has closed the connection.\n");
            // also close this connection 
            SSL_shutdown(ssl_conn);
        }
        ERR_print_errors_fp(stderr);
        return false;
    }

    uint32_t current_recv_size = 0;
    while (current_recv_size < msg_size) {
        read_stat = SSL_read(ssl_conn, data + current_recv_size,
            msg_size - current_recv_size);
        if (read_stat <= 0) {
            tool::Logging(my_name_.c_str(), "read the data fails. ret: %d\n",
                SSL_get_error(ssl_conn, read_stat));
            ERR_print_errors_fp(stderr);
            return false;
        }
        current_recv_size += read_stat;
    }
    recv_size = msg_size;

    return true;
}