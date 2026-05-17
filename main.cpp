#include "OrderBook.h"
#include "TCPServer.h"
#include <iostream>

const int DEFAULT_PORT = 8080;

int main() {
    OrderBook order_book;
    TCPServer server(order_book, DEFAULT_PORT);

    std::cout << "[MAIN] Starting HFT Core Phase 1..." << std::endl;
    try {
        server.start(); // This will block and run the server loop
    } catch (const std::exception& e) {
        std::cerr << "[MAIN] Server encountered an error: " << e.what() << std::endl;
        return 1;
    }
    // Server.start() currently runs an infinite loop, so this part is not reached
    // unless server.start() is modified to be stoppable.
    std::cout << "[MAIN] HFT Core Phase 1 shutting down." << std::endl; 
    return 0;
}











// #include "OrderBook.h" // Make sure this is OrderBook.h, not OrderBook.cpp
// // The definitions for OrderBook methods would be in OrderBook.cpp or in the OrderBook.h if you kept it that way.

// // Forward declare the functions if they are in the same file for this example,
// // or include their respective headers if they are in separate .cpp files.
// // For the UDP code block above, imagine those functions are available.
// void run_hft_core(OrderBook& book); // From the UDP code block
// void run_market_data_sender(); // From the UDP code block

// int main(int argc, char *argv[]) {
//     // This main function demonstrates how you might structure it.
//     // You'd compile MarketDataSender and HFTCore as separate executables.

//     if (argc > 1 && std::string(argv[1]) == "sender") {
//         run_market_data_sender();
//     } else if (argc > 1 && std::string(argv[1]) == "core") {
//         // Create an order book for a specific symbol
//         OrderBook main_book("MSFT"); // Example for Microsoft
//         run_hft_core(main_book);
//     } else {
//         std::cout << "Usage:" << std::endl;
//         std::cout << "  " << argv[0] << " sender   (to run the market data sender)" << std::endl;
//         std::cout << "  " << argv[0] << " core     (to run the HFT core)" << std::endl;
        
//         // --- Basic OrderBook Test (without UDP) ---
//         std::cout << "\n--- Running basic OrderBook test directly ---" << std::endl;
//         OrderBook test_book("TEST");
//         test_book.add_order(Order("TEST", OrderSide::BUY, 100.0, 10));
//         test_book.print_book();
//         test_book.add_order(Order("TEST", OrderSide::SELL, 101.0, 5));
//         test_book.print_book();
//         test_book.add_order(Order("TEST", OrderSide::BUY, 100.5, 3));
//         test_book.print_book();
//         // This order should match
//         std::vector<Trade> trades = test_book.add_order(Order("TEST", OrderSide::SELL, 100.0, 7)); 
//         if (!trades.empty()) {
//              std::cout << "Trades from SELL 100.0, 7:" << std::endl;
//              for(const auto& t : trades) {
//                  std::cout << "  Traded " << t.quantity << " @ " << t.price << std::endl;
//              }
//         }
//         test_book.print_book();
//          trades = test_book.add_order(Order("TEST", OrderSide::BUY, 101.0, 10)); // Should match remaining sell
//         if (!trades.empty()) {
//              std::cout << "Trades from BUY 101.0, 10:" << std::endl;
//              for(const auto& t : trades) {
//                  std::cout << "  Traded " << t.quantity << " @ " << t.price << std::endl;
//              }
//         }
//         test_book.print_book();
//     }

//     return 0;
// }
