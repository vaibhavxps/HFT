#include "OrderBook.h"
#include <iostream>
#include <algorithm> // For std::min
#include <iomanip>   // For std::fixed and std::setprecision

OrderBook::OrderBook() : next_order_id_(1) {}

std::vector<Trade> OrderBook::add_order(Order& order_param) {
    std::lock_guard<std::mutex> lock(book_mutex_);

    // Assign a unique engine order ID
    order_param.order_id = next_order_id_++;
    order_param.timestamp = std::chrono::system_clock::now(); // Set engine timestamp
    
    // Clear any trades from previous operations
    generated_trades_buffer_.clear();

    Order order = order_param; // Work with a copy locally

    if (order.side == OrderSide::BUY) {
        // Try to match with existing sell orders
        while (order.quantity > 0 && !sell_orders_.empty()) {
            auto best_sell_level_it = sell_orders_.begin(); // Lowest price sell orders
            double best_sell_price = best_sell_level_it->first;

            if (order.price >= best_sell_price) { // Match condition
                std::list<Order>& orders_at_level = best_sell_level_it->second;
                while (order.quantity > 0 && !orders_at_level.empty()) {
                    Order& sell_order = orders_at_level.front();
                    int trade_quantity = std::min(order.quantity, sell_order.quantity);

                    generated_trades_buffer_.emplace_back(order.symbol, sell_order.price, trade_quantity, order.order_id, sell_order.order_id);
                    std::cout << "[MATCHING ENGINE] Trade: " << order.symbol << " " << trade_quantity << " @ " << sell_order.price
                              << " (BuyID: " << order.order_id << ", SellID: " << sell_order.order_id << ")" << std::endl;

                    order.quantity -= trade_quantity;
                    order.filled_quantity += trade_quantity;
                    sell_order.quantity -= trade_quantity;
                    sell_order.filled_quantity += trade_quantity;

                    if (sell_order.quantity == 0) {
                        sell_order.status = OrderStatus::FILLED;
                        orders_at_level.pop_front();
                    } else {
                        sell_order.status = OrderStatus::PARTIALLY_FILLED;
                    }
                }
                if (orders_at_level.empty()) {
                    sell_orders_.erase(best_sell_level_it);
                }
            } else {
                break; // Buyer's price is too low for any sell order
            }
        }
        // If buy order is not fully filled, add remaining to buy_orders_
        if (order.quantity > 0) {
            order.status = (order.filled_quantity > 0) ? OrderStatus::PARTIALLY_FILLED : OrderStatus::NEW;
            buy_orders_[order.price].push_back(order);
        } else {
            order.status = OrderStatus::FILLED;
        }
    } else { // OrderSide::SELL
        // Try to match with existing buy orders
        while (order.quantity > 0 && !buy_orders_.empty()) {
            auto best_buy_level_it = buy_orders_.begin(); // Highest price buy orders
            double best_buy_price = best_buy_level_it->first;

            if (order.price <= best_buy_price) { // Match condition
                std::list<Order>& orders_at_level = best_buy_level_it->second;
                while (order.quantity > 0 && !orders_at_level.empty()) {
                    Order& buy_order = orders_at_level.front();
                    int trade_quantity = std::min(order.quantity, buy_order.quantity);

                    generated_trades_buffer_.emplace_back(order.symbol, buy_order.price, trade_quantity, buy_order.order_id, order.order_id);
                     std::cout << "[MATCHING ENGINE] Trade: " << order.symbol << " " << trade_quantity << " @ " << buy_order.price
                              << " (BuyID: " << buy_order.order_id << ", SellID: " << order.order_id << ")" << std::endl;

                    order.quantity -= trade_quantity;
                    order.filled_quantity += trade_quantity;
                    buy_order.quantity -= trade_quantity;
                    buy_order.filled_quantity += trade_quantity;

                    if (buy_order.quantity == 0) {
                        buy_order.status = OrderStatus::FILLED;
                        orders_at_level.pop_front();
                    } else {
                        buy_order.status = OrderStatus::PARTIALLY_FILLED;
                    }
                }
                if (orders_at_level.empty()) {
                    buy_orders_.erase(best_buy_level_it);
                }
            } else {
                break; // Seller's price is too high for any buy order
            }
        }
        // If sell order is not fully filled, add remaining to sell_orders_
        if (order.quantity > 0) {
            order.status = (order.filled_quantity > 0) ? OrderStatus::PARTIALLY_FILLED : OrderStatus::NEW;
            sell_orders_[order.price].push_back(order);
        } else {
            order.status = OrderStatus::FILLED;
        }
    }
    order_param = order; // Update the original order passed by reference with new status/id
    return generated_trades_buffer_;
}


void OrderBook::print_book() const {
    std::lock_guard<std::mutex> lock(book_mutex_);
    std::cout << "\n--- Order Book ---" << std::endl;
    std::cout << "SELL Orders (Price: Qty, ClientID, OrderID):" << std::endl;
    for (const auto& level : sell_orders_) {
        std::cout << std::fixed << std::setprecision(2) << "  Price " << level.first << ": ";
        for (const auto& order : level.second) {
            std::cout << order.quantity << " (" << order.client_order_id << ", " << order.order_id << ") | ";
        }
        std::cout << std::endl;
    }
    std::cout << "BUY Orders (Price: Qty, ClientID, OrderID):" << std::endl;
    for (const auto& level : buy_orders_) {
        std::cout << std::fixed << std::setprecision(2) << "  Price " << level.first << ": ";
        for (const auto& order : level.second) {
             std::cout << order.quantity << " (" << order.client_order_id << ", " << order.order_id << ") | ";
        }
        std::cout << std::endl;
    }
    std::cout << "------------------" << std::endl;
}