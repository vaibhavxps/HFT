// #include "TCPServer.h"
// #include "Order.h"
// #include <iostream>
// #include <string>
// #include <vector>
// #include <sstream>    // For string splitting
// #include <cstring>    // For strerror, memset
// #include <stdexcept>  // For std::invalid_argument, std::out_of_range

// // POSIX Socket includes
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h> // For close(), read(), write()
// #include <arpa/inet.h> // for inet_ntoa

// TCPServer::TCPServer(OrderBook& book, int port) : order_book_(book), port_(port), server_fd_(-1) {}

// TCPServer::~TCPServer() {
//     if (server_fd_ != -1) {
//         close(server_fd_);
//     }
// }

// // Helper to split string by delimiter
// std::vector<std::string> split(const std::string& s, char delimiter) {
//     std::vector<std::string> tokens;
//     std::string token;
//     std::istringstream tokenStream(s);
//     while (std::getline(tokenStream, token, delimiter)) {
//         tokens.push_back(token);
//     }
//     return tokens;
// }


// Order TCPServer::parse_order_message(const std::string& msg_str, std::string& error_msg) {
//     // Expected format: SYMBOL,SIDE,PRICE,QUANTITY,CLIENT_ORDER_ID
//     // Example: "AAPL,BUY,150.50,100,C001"
//     std::vector<std::string> tokens = split(msg_str, ',');
//     if (tokens.size() != 5) {
//         error_msg = "ERR:Invalid message format. Expected 5 fields, got " + std::to_string(tokens.size());
//         return Order(0, "", "", OrderSide::BUY, 0.0, 0); // Dummy order
//     }

//     try {
//         std::string symbol = tokens[0];
//         OrderSide side;
//         if (tokens[1] == "BUY") side = OrderSide::BUY;
//         else if (tokens[1] == "SELL") side = OrderSide::SELL;
//         else {
//             error_msg = "ERR:Invalid side. Expected BUY or SELL.";
//             return Order(0, "", "", OrderSide::BUY, 0.0, 0);
//         }
//         double price = std::stod(tokens[2]);
//         int quantity = std::stoi(tokens[3]);
//         std::string client_order_id = tokens[4];

//         if (price <= 0 || quantity <= 0) {
//             error_msg = "ERR:Price and quantity must be positive.";
//             return Order(0, "", "", OrderSide::BUY, 0.0, 0);
//         }

//         // Order ID 0 is a placeholder, real ID assigned by OrderBook
//         return Order(0, client_order_id, symbol, side, price, quantity);

//     } catch (const std::invalid_argument& ia) {
//         error_msg = "ERR:Invalid number format for price or quantity. " + std::string(ia.what());
//         return Order(0, "", "", OrderSide::BUY, 0.0, 0);
//     } catch (const std::out_of_range& oor) {
//         error_msg = "ERR:Number out of range for price or quantity. " + std::string(oor.what());
//         return Order(0, "", "", OrderSide::BUY, 0.0, 0);
//     }
// }


// void TCPServer::handle_client(int client_socket) {
//     char buffer[1024] = {0};
//     std::cout << "[TCP SERVER] Handling new client connection." << std::endl;

//     // For this simple server, we process one message then close.
//     // A more robust server would loop to handle multiple messages per connection.
//     ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     if (bytes_read > 0) {
//         buffer[bytes_read] = '\0'; // Null-terminate the received string
//         std::string client_message(buffer);
//         // Trim newline characters if present (common from netcat or some clients)
//         client_message.erase(client_message.find_last_not_of("\r\n") + 1);

//         std::cout << "[TCP SERVER] Received from client: \"" << client_message << "\"" << std::endl;

//         std::string error_msg;
//         Order new_order = parse_order_message(client_message, error_msg);

//         std::string response;
//         if (!error_msg.empty()) {
//             response = error_msg + "\n";
//         } else {
//             std::vector<Trade> trades = order_book_.add_order(new_order); // new_order is updated with engine_id here
            
//             std::stringstream ss_response;
//             ss_response << "ACK:" << new_order.client_order_id << ":ENGINE_ID:" << new_order.order_id 
//                         << ":STATUS:" << (new_order.status == OrderStatus::FILLED ? "FILLED" : (new_order.status == OrderStatus::PARTIALLY_FILLED ? "PARTIALLY_FILLED" : "NEW"))
//                         << ":FILLED_QTY:" << new_order.filled_quantity << "/" << (new_order.quantity + new_order.filled_quantity) // original quantity
//                         << "\n";

//             if (!trades.empty()) {
//                 for (const auto& trade : trades) {
//                     ss_response << "TRADE:" << trade.symbol << "," << trade.quantity << "@" << trade.price
//                                 << ",BUY_ID:" << trade.buy_order_id << ",SELL_ID:" << trade.sell_order_id << "\n";
//                 }
//             }
//             response = ss_response.str();
//         }
        
//         write(client_socket, response.c_str(), response.length());
//         std::cout << "[TCP SERVER] Sent to client: \"" << response << "\"" << std::endl;
//         order_book_.print_book(); // Display book status after operation

//     } else if (bytes_read == 0) {
//         std::cout << "[TCP SERVER] Client disconnected." << std::endl;
//     } else {
//         std::cerr << "[TCP SERVER] Error reading from socket: " << strerror(errno) << std::endl;
//     }

//     close(client_socket);
//     std::cout << "[TCP SERVER] Client connection closed." << std::endl;
// }


// void TCPServer::start() {
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     // Creating socket file descriptor
//     if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("[TCP SERVER] Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // Forcefully attaching socket to the port
//     if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//         perror("[TCP SERVER] setsockopt failed");
//         close(server_fd_);
//         exit(EXIT_FAILURE);
//     }
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
//     address.sin_port = htons(port_);

//     // Binding the socket to the network address and port
//     if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[TCP SERVER] Bind failed");
//         close(server_fd_);
//         exit(EXIT_FAILURE);
//     }

//     // Start listening for incoming connections
//     if (listen(server_fd_, 3) < 0) { // Backlog of 3 pending connections
//         perror("[TCP SERVER] Listen failed");
//         close(server_fd_);
//         exit(EXIT_FAILURE);
//     }

//     std::cout << "[TCP SERVER] Listening on port " << port_ << std::endl;

//     while (true) { // Main accept loop
//         int client_socket;
//         struct sockaddr_in client_address;
//         socklen_t client_addrlen = sizeof(client_address);

//         if ((client_socket = accept(server_fd_, (struct sockaddr *)&client_address, &client_addrlen)) < 0) {
//             perror("[TCP SERVER] Accept failed");
//             // Optionally continue or exit, for now, we log and try to continue
//             // If server_fd_ was closed by a signal or something, this loop should terminate.
//             // For simplicity, we don't handle that here.
//             continue; 
//         }
        
//         char client_ip[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
//         int client_port = ntohs(client_address.sin_port);
//         std::cout << "[TCP SERVER] Accepted connection from " << client_ip << ":" << client_port << std::endl;

//         handle_client(client_socket); // Handle this client (iterative server)
//     }
//     // Cleanup is in destructor, but this loop is infinite for now.
//     // A real server would have a way to gracefully shut down.
// }























// // // Common includes for both sender and receiver
// // #include <iostream>
// // #include <string>
// // #include <vector>
// // #include <cstring> // For memset, strcpy, strerror
// // #include <cstdlib> // For std::stod, std::stoi, std::exit
// // #include <sstream> // For parsing strings

// // // Networking includes (differ slightly for Windows vs. Linux/macOS)
// // #ifdef _WIN32
// //     #include <winsock2.h>
// //     #include <ws2tcpip.h> // For inet_pton, etc.
// //     #pragma comment(lib, "ws2_32.lib") // Link with Winsock library
// // #else // Linux/macOS
// //     #include <sys/types.h>
// //     #include <sys/socket.h>
// //     #include <netinet/in.h>
// //     #include <arpa/inet.h> // For inet_pton, htons, etc.
// //     #include <unistd.h>    // For close()
// //     // Define SOCKET and INVALID_SOCKET for compatibility if not on Windows
// //     typedef int SOCKET;
// //     const int INVALID_SOCKET = -1;
// //     const int SOCKET_ERROR = -1;
// //     #define closesocket close // POSIX uses close()
// // #endif

// // const char* SERVER_IP = "127.0.0.1"; // Loopback IP for local testing
// // const int PORT = 8888;
// // const int BUFFER_SIZE = 1024;

// // // Helper function to initialize Winsock on Windows
// // void initialize_winsock() {
// // #ifdef _WIN32
// //     WSADATA wsaData;
// //     int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
// //     if (result != 0) {
// //         std::cerr << "WSAStartup failed: " << result << std::endl;
// //         std::exit(1);
// //     }
// // #endif
// // }

// // // Helper function to clean up Winsock on Windows
// // void cleanup_winsock() {
// // #ifdef _WIN32
// //     WSACleanup();
// // #endif
// // }

// // // --- MarketDataSimulator.cpp (Sender) ---
// // // This would be compiled as a separate program.
// // // To run: ./market_data_sender

// // void run_market_data_sender() {
// //     initialize_winsock();

// //     SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// //     if (sock == INVALID_SOCKET) {
// //         std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
// //         cleanup_winsock();
// //         return;
// //     }

// //     sockaddr_in server_address;
// //     server_address.sin_family = AF_INET;
// //     server_address.sin_port = htons(PORT);
// //     // Convert IP address from text to binary form
// //     if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
// //         std::cerr << "Invalid address/ Address not supported" << std::endl;
// //         closesocket(sock);
// //         cleanup_winsock();
// //         return;
// //     }
    
// //     std::cout << "Market Data Sender started. Type messages like:" << std::endl;
// //     std::cout << "SYMBOL,SIDE,PRICE,QUANTITY (e.g., MSFT,BUY,150.75,100)" << std::endl;
// //     std::cout << "Type 'exit' to quit." << std::endl;

// //     std::string line;
// //     char buffer[BUFFER_SIZE];

// //     while (true) {
// //         std::cout << "> ";
// //         std::getline(std::cin, line);

// //         if (line == "exit") {
// //             break;
// //         }

// //         if (line.length() >= BUFFER_SIZE) {
// //             std::cerr << "Message too long!" << std::endl;
// //             continue;
// //         }
// //         strcpy(buffer, line.c_str()); // Be careful with strcpy, ensure buffer is large enough

// //         if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
// //             std::cerr << "sendto failed: " << strerror(errno) << std::endl;
// //         } else {
// //             std::cout << "Sent: " << line << std::endl;
// //         }
// //     }

// //     closesocket(sock);
// //     cleanup_winsock();
// //     std::cout << "Market Data Sender stopped." << std::endl;
// // }


// // // --- HFTCore.cpp (Receiver) ---
// // // This would be compiled as another separate program, linking with Order.h and OrderBook.h/cpp
// // // To run: ./hft_core
// // // #include "OrderBook.h" // Assume OrderBook.h and Order.h are available

// // // Helper to parse the simple message format: "SYMBOL,SIDE,PRICE,QTY"
// // Order* parse_market_data_message(const std::string& msg_str) {
// //     std::stringstream ss(msg_str);
// //     std::string segment;
// //     std::vector<std::string> segments;

// //     while(std::getline(ss, segment, ',')) {
// //        segments.push_back(segment);
// //     }

// //     if (segments.size() != 4) {
// //         std::cerr << "Invalid message format: " << msg_str << std::endl;
// //         return nullptr;
// //     }

// //     try {
// //         std::string symbol = segments[0];
// //         OrderSide side;
// //         if (segments[1] == "BUY") side = OrderSide::BUY;
// //         else if (segments[1] == "SELL") side = OrderSide::SELL;
// //         else {
// //             std::cerr << "Invalid order side: " << segments[1] << std::endl;
// //             return nullptr;
// //         }
// //         double price = std::stod(segments[2]);
// //         int quantity = std::stoi(segments[3]);

// //         return new Order(symbol, side, price, quantity); // Caller must delete!
// //     } catch (const std::exception& e) {
// //         std::cerr << "Error parsing message '" << msg_str << "': " << e.what() << std::endl;
// //         return nullptr;
// //     }
// // }


// // void run_hft_core(OrderBook& book) { // Pass the order book by reference
// //     initialize_winsock();

// //     SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// //     if (sock == INVALID_SOCKET) {
// //         std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
// //         cleanup_winsock();
// //         return;
// //     }

// //     sockaddr_in server_address;
// //     server_address.sin_family = AF_INET;
// //     server_address.sin_port = htons(PORT);
// //     server_address.sin_addr.s_addr = INADDR_ANY; // Listen on any IP address

// //     if (bind(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
// //         std::cerr << "Bind failed: " << strerror(errno) << std::endl;
// //         closesocket(sock);
// //         cleanup_winsock();
// //         return;
// //     }

// //     std::cout << "HFT Core listening on port " << PORT << "..." << std::endl;

// //     char buffer[BUFFER_SIZE];
// //     sockaddr_in client_address;
// //     socklen_t client_addr_len = sizeof(client_address); // socklen_t for POSIX

// //     while (true) {
// //         memset(buffer, 0, BUFFER_SIZE); // Clear the buffer
// //         int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE -1, 0, (struct sockaddr*)&client_address, &client_addr_len);

// //         if (bytes_received == SOCKET_ERROR) {
// //             std::cerr << "recvfrom failed: " << strerror(errno) << std::endl;
// //             continue;
// //         }
        
// //         buffer[bytes_received] = '\0'; // Null-terminate the received data
// //         std::string received_message(buffer);

// //         // --- Latency Measurement Point 1: Message Received ---
// //         auto t_msg_received_by_core = std::chrono::high_resolution_clock::now();

// //         std::cout << "Received UDP: " << received_message << std::endl;

// //         Order* order_ptr = parse_market_data_message(received_message);
// //         if (order_ptr) {
// //             // --- Latency Measurement Point 2: Before adding to book ---
// //             // (order_ptr->timestamp was set at order creation by sender or parser)
// //             // The time difference between order_ptr->timestamp and t_msg_received_by_core
// //             // could represent (simulated network + parsing) latency.
            
// //             std::cout << "Processing order ID: " << order_ptr->id << std::endl;
// //             std::vector<Trade> trades = book.add_order(*order_ptr); // Add to the order book

// //             // --- Latency Measurement Point 3: After order processed by book ---
// //             auto t_order_processed = std::chrono::high_resolution_clock::now();
// //             auto processing_duration = std::chrono::duration_cast<std::chrono::microseconds>(t_order_processed - order_ptr->timestamp);
// //             // Note: order_ptr->timestamp is from when the Order object was created in parse_market_data_message.
// //             // If you want full end-to-end from sender, the sender would need to put a timestamp in the message.

// //             std::cout << "Order " << order_ptr->id << " processing (creation to match attempt) took: "
// //                       << processing_duration.count() << " microseconds." << std::endl;

// //             if (!trades.empty()) {
// //                 std::cout << "Trades executed:" << std::endl;
// //                 for (const auto& trade : trades) {
// //                     std::cout << "  - Trade: " << trade.quantity << " of " << trade.symbol
// //                               << " @ " << trade.price << " (BuyID: " << trade.buy_order_id
// //                               << ", SellID: " << trade.sell_order_id << ")" << std::endl;
// //                 }
// //             }
// //             book.print_book(); // Print book state after changes
// //             delete order_ptr; // IMPORTANT: Clean up the dynamically allocated order
// //         }
// //     }

// //     closesocket(sock);
// //     cleanup_winsock();
// // }




#include "TCPServer.h"
#include "Order.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>    // For string splitting
#include <cstring>    // For strerror, memset, strlen (strerror on POSIX)
#include <stdexcept>  // For std::invalid_argument, std::out_of_range

// Platform-specific error handling and socket closing
#ifdef _WIN32
    #define GET_LAST_SOCKET_ERROR() WSAGetLastError()
    #define CLOSE_SOCKET(s) closesocket(s)
    #define IS_SOCKET_VALID(s) (s != INVALID_SOCKET)
#else
    #define GET_LAST_SOCKET_ERROR() errno
    #define CLOSE_SOCKET(s) close(s)
    #define IS_SOCKET_VALID(s) (s >= 0) // POSIX sockets are non-negative on success
#endif

TCPServer::TCPServer(OrderBook& book, int port) 
    : order_book_(book), port_(port), server_fd_(INVALID_SOCKET) {
#ifdef _WIN32
    winsock_initialized_ = false;
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "[TCP SERVER] WSAStartup failed: " << iResult << std::endl;
        // Server will fail to start if socket creation fails later
    } else {
        winsock_initialized_ = true;
    }
#endif
}

TCPServer::~TCPServer() {
    if (IS_SOCKET_VALID(server_fd_)) {
        CLOSE_SOCKET(server_fd_);
        server_fd_ = INVALID_SOCKET;
    }
#ifdef _WIN32
    if (winsock_initialized_) {
        WSACleanup();
    }
#endif
}

// Helper to split string by delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

Order TCPServer::parse_order_message(const std::string& msg_str, std::string& error_msg) {
    std::vector<std::string> tokens = split(msg_str, ',');
    if (tokens.size() != 5) {
        error_msg = "ERR:Invalid message format. Expected 5 fields, got " + std::to_string(tokens.size());
        return Order(0, "", "", OrderSide::BUY, 0.0, 0); 
    }
    try {
        std::string symbol = tokens[0];
        OrderSide side;
        if (tokens[1] == "BUY") side = OrderSide::BUY;
        else if (tokens[1] == "SELL") side = OrderSide::SELL;
        else {
            error_msg = "ERR:Invalid side. Expected BUY or SELL.";
            return Order(0, "", "", OrderSide::BUY, 0.0, 0);
        }
        double price = std::stod(tokens[2]);
        int quantity = std::stoi(tokens[3]);
        std::string client_order_id = tokens[4];

        if (price <= 0 || quantity <= 0) {
            error_msg = "ERR:Price and quantity must be positive.";
            return Order(0, "", "", OrderSide::BUY, 0.0, 0);
        }
        return Order(0, client_order_id, symbol, side, price, quantity);

    } catch (const std::invalid_argument& ia) {
        error_msg = "ERR:Invalid number format: " + std::string(ia.what());
        return Order(0, "", "", OrderSide::BUY, 0.0, 0);
    } catch (const std::out_of_range& oor) {
        error_msg = "ERR:Number out of range: " + std::string(oor.what());
        return Order(0, "", "", OrderSide::BUY, 0.0, 0);
    }
}

void TCPServer::handle_client(socket_t client_socket) {
    char buffer[1024] = {0};
    std::cout << "[TCP SERVER] Handling new client connection." << std::endl;

    int bytes_read; // recv/read return int or ssize_t. int is safer for cross-platform.
#ifdef _WIN32
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#else
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
#endif

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string client_message(buffer);
        client_message.erase(client_message.find_last_not_of("\r\n") + 1);
        std::cout << "[TCP SERVER] Received from client: \"" << client_message << "\"" << std::endl;

        std::string error_msg;
        Order new_order = parse_order_message(client_message, error_msg);
        std::string response;

        if (!error_msg.empty()) {
            response = error_msg + "\n";
        } else {
            std::vector<Trade> trades = order_book_.add_order(new_order);
            std::stringstream ss_response;
            ss_response << "ACK:" << new_order.client_order_id << ":ENGINE_ID:" << new_order.order_id 
                        << ":STATUS:" << (new_order.status == OrderStatus::FILLED ? "FILLED" : (new_order.status == OrderStatus::PARTIALLY_FILLED ? "PARTIALLY_FILLED" : "NEW"))
                        << ":FILLED_QTY:" << new_order.filled_quantity << "/" << (new_order.quantity + new_order.filled_quantity)
                        << "\n";
            if (!trades.empty()) {
                for (const auto& trade : trades) {
                    ss_response << "TRADE:" << trade.symbol << "," << trade.quantity << "@" << trade.price
                                << ",BUY_ID:" << trade.buy_order_id << ",SELL_ID:" << trade.sell_order_id << "\n";
                }
            }
            response = ss_response.str();
        }
        
        int bytes_sent;
#ifdef _WIN32
        bytes_sent = send(client_socket, response.c_str(), static_cast<int>(response.length()), 0);
#else
        bytes_sent = write(client_socket, response.c_str(), response.length());
#endif
        if (bytes_sent == SOCKET_ERROR) {
             std::cerr << "[TCP SERVER] Send failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
        } else {
            std::cout << "[TCP SERVER] Sent to client: \"" << response << "\"" << std::endl;
        }
        order_book_.print_book();

    } else if (bytes_read == 0) {
        std::cout << "[TCP SERVER] Client disconnected." << std::endl;
    } else { // bytes_read < 0 (SOCKET_ERROR)
        std::cerr << "[TCP SERVER] Recv/Read failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
    }

    CLOSE_SOCKET(client_socket);
    std::cout << "[TCP SERVER] Client connection closed." << std::endl;
}

void TCPServer::start() {
#ifdef _WIN32
    if (!winsock_initialized_) {
        std::cerr << "[TCP SERVER] Winsock not initialized. Cannot start server." << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

    struct sockaddr_in address;
    server_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Use IPPROTO_TCP for clarity
    if (!IS_SOCKET_VALID(server_fd_)) {
        std::cerr << "[TCP SERVER] Socket creation failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    // On Windows, setsockopt optval for SO_REUSEADDR is often char* for a boolean
    char opt_true = 1; 
    const char* p_opt_val = &opt_true; 
    int opt_len = sizeof(opt_true);
#else
    int opt_true = 1;
    const int* p_opt_val = &opt_true;
    socklen_t opt_len = sizeof(opt_true);
#endif
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, p_opt_val, opt_len) == SOCKET_ERROR) {
        std::cerr << "[TCP SERVER] setsockopt(SO_REUSEADDR) failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(server_fd_);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<u_short>(port_));

    if (bind(server_fd_, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "[TCP SERVER] Bind failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(server_fd_);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd_, 3) == SOCKET_ERROR) { // Backlog of 3
        std::cerr << "[TCP SERVER] Listen failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
        CLOSE_SOCKET(server_fd_);
        exit(EXIT_FAILURE);
    }

    std::cout << "[TCP SERVER] Listening on port " << port_ << std::endl;

    while (true) {
        socket_t client_socket;
        struct sockaddr_in client_address;
#ifdef _WIN32
        int client_addrlen = sizeof(client_address);
#else
        socklen_t client_addrlen = sizeof(client_address);
#endif

        client_socket = accept(server_fd_, (struct sockaddr *)&client_address, &client_addrlen);
        if (!IS_SOCKET_VALID(client_socket)) {
            std::cerr << "[TCP SERVER] Accept failed with error: " << GET_LAST_SOCKET_ERROR() << std::endl;
            continue; 
        }
        
        char client_ip_str[INET_ADDRSTRLEN];
#ifdef _WIN32
        // inet_ntop is preferred and available via Ws2tcpip.h (included)
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip_str, INET_ADDRSTRLEN);
#else
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip_str, INET_ADDRSTRLEN);
#endif
        std::cout << "[TCP SERVER] Accepted connection from " << client_ip_str << ":" << ntohs(client_address.sin_port) << std::endl;
        
        handle_client(client_socket);
    }
}