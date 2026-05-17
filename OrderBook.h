#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"
#include <map>
#include <list>
#include <vector>
#include <mutex>    // For thread safety
#include <string>
#include <functional> // For std::greater

class OrderBook {
public:
    OrderBook();
    // Adds an order to the book and tries to match it.
    // Returns a list of trades that occurred.
    std::vector<Trade> add_order(Order& order); // Pass by non-const ref to update order_id
    void print_book() const; // For debugging

private:
    // Buy orders: highest price first (std::greater)
    std::map<double, std::list<Order>, std::greater<double>> buy_orders_;
    // Sell orders: lowest price first (default std::less)
    std::map<double, std::list<Order>> sell_orders_;

    //std::mutex book_mutex_; // To protect access to order books
    mutable std::mutex book_mutex_; // To protect access to order books
    long long next_order_id_;

    void match_orders(); // Internal matching logic
    std::vector<Trade> generated_trades_buffer_; // To store trades from one add_order call
};

#endif // ORDER_BOOK_H









// // OrderBook.h
// #ifndef ORDER_BOOK_H
// #define ORDER_BOOK_H

// #include "Order.h" // We need the Order struct
// #include <map>
// #include <list>   // To store orders at the same price level
// #include <vector> // To return matched trades
// #include <iostream> // For printing

// // Represents a trade that occurred
// struct Trade {
//     long long buy_order_id;
//     long long sell_order_id;
//     std::string symbol;
//     double price;
//     int quantity;
//     std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;

//     Trade(long long b_id, long long s_id, const std::string& sym, double p, int q)
//         : buy_order_id(b_id), sell_order_id(s_id), symbol(sym), price(p), quantity(q) {
//         timestamp = std::chrono::high_resolution_clock::now();
//     }
// };

// class OrderBook {
// public:
//     OrderBook(const std::string& symbol) : book_symbol(symbol) {}

//     // Function to add an order to the book
//     // It will also try to match orders immediately after adding.
//     std::vector<Trade> add_order(Order order);

//     // For debugging: print the current state of the order book
//     void print_book() const;

// private:
//     std::string book_symbol; // Each order book is for a specific symbol

//     // Buy orders: map<price, list_of_orders_at_that_price>
//     // We want the highest bid price first, so we use std::greater<double>
//     std::map<double, std::list<Order>, std::greater<double>> bids;

//     // Sell orders: map<price, list_of_orders_at_that_price>
//     // We want the lowest ask price first, so we use std::less<double> (default for map)
//     std::map<double, std::list<Order>> asks;

//     // Internal function to try and match orders
//     std::vector<Trade> match_orders();
// };

// #endif // ORDER_BOOK_H

// // --- Separate conceptual file: OrderBook.cpp ---
// // (In a real project, this would be in OrderBook.cpp)
// // For simplicity here, I'm putting it in the same block.

// // #include "OrderBook.h" // Already included if this were a separate file

// std::vector<Trade> OrderBook::add_order(Order order) {
//     if (order.symbol != book_symbol) {
//         // In a real system, you might have multiple books or route this
//         // For now, we'll just ignore orders for other symbols
//         std::cerr << "Order symbol " << order.symbol << " does not match book symbol " << book_symbol << std::endl;
//         return {}; // Return an empty vector of trades
//     }

//     // Record the time we officially accept the order into our system
//     auto t_received_by_book = std::chrono::high_resolution_clock::now();
//     // You could compare order.timestamp (creation) with t_received_by_book for network latency later

//     if (order.side == OrderSide::BUY) {
//         bids[order.price].push_back(order);
//         std::cout << "Added BUY order: " << order.id << " " << order.quantity << "@" << order.price << std::endl;
//     } else { // OrderSide::SELL
//         asks[order.price].push_back(order);
//         std::cout << "Added SELL order: " << order.id << " " << order.quantity << "@" << order.price << std::endl;
//     }

//     // After adding an order, always try to match
//     return match_orders();
// }

// std::vector<Trade> OrderBook::match_orders() {
//     std::vector<Trade> trades_executed;

//     // Keep matching as long as there's a potential match
//     // Best bid price must be >= best ask price
//     while (!bids.empty() && !asks.empty() && bids.begin()->first >= asks.begin()->first) {
        
//         // Get the best bid and best ask price levels
//         auto& best_bid_level = bids.begin()->second; // list of orders at highest bid price
//         auto& best_ask_level = asks.begin()->second; // list of orders at lowest ask price

//         // Get the first order from each list (oldest order at that price level)
//         Order& buy_order = best_bid_level.front();
//         Order& sell_order = best_ask_level.front();

//         // Determine trade quantity (minimum of buy and sell order quantities)
//         int trade_quantity = std::min(buy_order.quantity, sell_order.quantity);
        
//         // Determine trade price (typically the price of the order that was already in the book - the "standing" order)
//         // This is a common convention. If buy_order arrived and hit an existing sell_order, trade at sell_order's price.
//         // If sell_order arrived and hit an existing buy_order, trade at buy_order's price.
//         // For simplicity, if timestamps are equal (unlikely with high_resolution_clock but possible if processed fast),
//         // we can default to one or the other (e.g. ask price).
//         // A common rule is that the price of the order that was resting on the book determines the trade price.
//         double trade_price;
//         if (buy_order.timestamp < sell_order.timestamp) { // buy_order was resting, sell_order is incoming
//             trade_price = buy_order.price;
//         } else if (sell_order.timestamp < buy_order.timestamp) { // sell_order was resting, buy_order is incoming
//             trade_price = sell_order.price;
//         } else {
//             // If timestamps are identical (e.g. simulated or very fast internal),
//             // or if both are "new" and crossing, a common rule is to use the price of the order
//             // that makes the market (the standing order). If both are aggressive and cross,
//             // the exchange might have specific rules. For simplicity, let's use the ask price if they cross.
//             // In our current logic, one order is added, then matching is called.
//             // So one of them is "newer". The older one's price is typically used.
//              trade_price = asks.begin()->first; // Could also be bids.begin()->first, depends on exchange rules.
//                                               // Let's assume the incoming order takes the price of the resting order.
//                                               // If buy_order is newer, it takes sell_order.price.
//                                               // If sell_order is newer, it takes buy_order.price.
//                                               // This is implicitly handled by the logic above.
//                                               // If they are truly simultaneous and cross, it's more complex.
//                                               // For now, the ask price is a simple choice if they are at the same price level.
//         }
//         // More robust: if bids.begin()->first == asks.begin()->first, trade_price is that price.
//         // If bids.begin()->first > asks.begin()->first (a cross):
//         // If new order is BUY, trade_price = asks.begin()->first (existing sell order price)
//         // If new order is SELL, trade_price = bids.begin()->first (existing buy order price)
//         // Our current logic: an order is added, then match_orders() is called.
//         // The order just added is the "aggressive" one. The one on the other side is "passive".
//         // Trade occurs at the passive order's price.
//         if (buy_order.id == next_order_id -1) { // buy_order is the one just added
//              trade_price = sell_order.price;
//         } else { // sell_order is the one just added
//              trade_price = buy_order.price;
//         }


//         std::cout << "MATCH: " << trade_quantity << " shares of " << book_symbol
//                   << " between BUY order " << buy_order.id << " and SELL order " << sell_order.id
//                   << " at price " << trade_price << std::endl;
        
//         trades_executed.emplace_back(buy_order.id, sell_order.id, book_symbol, trade_price, trade_quantity);

//         // Update quantities
//         buy_order.quantity -= trade_quantity;
//         sell_order.quantity -= trade_quantity;

//         // If an order is fully filled, remove it
//         if (buy_order.quantity == 0) {
//             best_bid_level.pop_front();
//         }
//         if (sell_order.quantity == 0) {
//             best_ask_level.pop_front();
//         }

//         // If a price level is now empty, remove it from the map
//         if (best_bid_level.empty()) {
//             bids.erase(bids.begin());
//         }
//         if (best_ask_level.empty()) {
//             asks.erase(asks.begin());
//         }
//     }
//     return trades_executed;
// }

// void OrderBook::print_book() const {
//     std::cout << "\n--- ORDER BOOK FOR " << book_symbol << " ---" << std::endl;
//     std::cout << "ASKS (Price: Quantity@ID, ...):" << std::endl;
//     // Asks are sorted lowest to highest price. Iterate normally.
//     for (const auto& pair : asks) {
//         std::cout << pair.first << ": ";
//         for (const auto& order : pair.second) {
//             std::cout << order.quantity << "@" << order.id << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::cout << "--------------------------" << std::endl;
//     // Bids are sorted highest to lowest price. Iterate normally (as map is already sorted that way).
//     for (const auto& pair : bids) {
//         std::cout << pair.first << ": ";
//         for (const auto& order : pair.second) {
//             std::cout << order.quantity << "@" << order.id << " ";
//         }
//         std::cout << std::endl;
//     }
//     std::cout << "BIDS (Price: Quantity@ID, ...):" << std::endl; // Just to make it clear
//     std::cout << "--- END OF BOOK ---" << std::endl;
// }
