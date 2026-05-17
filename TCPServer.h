// #ifndef TCP_SERVER_H
// #define TCP_SERVER_H

// #include "OrderBook.h"
// #include <string>
// #include <vector> // For parsing

// class TCPServer {
// public:
//     TCPServer(OrderBook& book, int port);
//     ~TCPServer();
//     void start(); // Start listening for connections

// private:
//     OrderBook& order_book_;
//     int port_;
//     int server_fd_; // Server socket file descriptor

//     void handle_client(int client_socket);
//     Order parse_order_message(const std::string& msg_str, std::string& error_msg); // Returns a dummy order on parse error
// };

// #endif // TCP_SERVER_H


#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "OrderBook.h"
#include <string>
#include <vector> // For parsing

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Link with Ws2_32.lib (for MSVC, MinGW uses linker flag)
    // #pragma comment(lib, "Ws2_32.lib") // Not strictly needed for MinGW if LDFLAGS is set
    typedef SOCKET socket_t; // Use SOCKET type for Windows
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>    // For close(), read(), write()
    #include <arpa/inet.h> // for inet_ntop
    #include <errno.h>     // For errno
    typedef int socket_t;  // Use int for POSIX
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
#endif

class TCPServer {
public:
    TCPServer(OrderBook& book, int port);
    ~TCPServer();
    void start(); // Start listening for connections

private:
    OrderBook& order_book_;
    int port_;
    socket_t server_fd_; // Server socket file descriptor
#ifdef _WIN32
    bool winsock_initialized_; // Track WSAStartup
#endif

    void handle_client(socket_t client_socket);
    Order parse_order_message(const std::string& msg_str, std::string& error_msg);
};

#endif // TCP_SERVER_H