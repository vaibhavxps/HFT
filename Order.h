#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <chrono> // For timestamp

// Enum for order side
enum class OrderSide {
    BUY,
    SELL
};

// Enum for order status (simplified)
enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED // Not implemented in this phase
};

struct Order {
    long long order_id;          // Unique ID assigned by the matching engine
    std::string client_order_id; // ID from the client
    std::string symbol;
    OrderSide side;
    double price;
    int quantity;
    int filled_quantity;
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp; // Time order was received by engine

    Order(long long id, const std::string& client_id, const std::string& sym, OrderSide s, double p, int qty)
        : order_id(id), client_order_id(client_id), symbol(sym), side(s), price(p), quantity(qty),
          filled_quantity(0), status(OrderStatus::NEW), timestamp(std::chrono::system_clock::now()) {}

    // Helper to convert side to string
    static std::string side_to_string(OrderSide s) {
        return s == OrderSide::BUY ? "BUY" : "SELL";
    }
};

struct Trade {
    std::string symbol;
    double price;
    int quantity;
    long long buy_order_id;
    long long sell_order_id;
    std::chrono::system_clock::time_point timestamp;

    Trade(const std::string& sym, double p, int qty, long long b_id, long long s_id)
        : symbol(sym), price(p), quantity(qty), buy_order_id(b_id), sell_order_id(s_id),
          timestamp(std::chrono::system_clock::now()) {}
};

#endif // ORDER_H









// #ifndef ORDER_H
// #define ORDER_H

// #include <string>
// #include <chrono> // For timestamps

// // To keep things simple, we'll define Buy/Sell as an enum
// enum class OrderSide {
//     BUY,
//     SELL
// };

// // A unique ID for each order. In a real system, this would be more robust.
// // For now, a simple integer counter will do.
// long long next_order_id = 1;

// struct Order {
//     long long id;             // Unique order ID
//     std::string symbol;       // e.g., "AAPL", "GOOG"
//     OrderSide side;           // BUY or SELL
//     double price;             // Order price
//     int quantity;             // Number of shares
//     std::chrono::time_point<std::chrono::high_resolution_clock> timestamp; // When the order was created/received

//     // Constructor to make creating orders easier
//     Order(const std::string& sym, OrderSide s, double p, int qty)
//         : symbol(sym), side(s), price(p), quantity(qty) {
//         id = next_order_id++; // Assign a unique ID
//         timestamp = std::chrono::high_resolution_clock::now(); // Record creation time
//     }

//     // Helper to print order side
//     std::string side_to_string() const {
//         return (side == OrderSide::BUY) ? "BUY" : "SELL";
//     }
// };

// #endif // ORDER_H
