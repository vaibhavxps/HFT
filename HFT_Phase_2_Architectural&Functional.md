# HFT Simulator: Architectural & Functional Deep Dive

> **Language:** C++17 · **Architecture:** 3-Thread Asymmetric Pipeline · **Platform:** Windows (Winsock2) / Linux (POSIX)

---

## Table of Contents

- [System Overview](#1-system-overview)
- [Thread Topology](#2-thread-topology)
- [File Architecture](#3-file-architecture)
- [Data Structures](#4-data-structures)
- [Inter-Thread Communication](#5-inter-thread-communication)
- [Pipeline Stages — Detailed](#6-pipeline-stages--detailed)
- [Matching Algorithm](#7-matching-algorithm)
- [Network Protocol](#8-network-protocol)
- [Performance Characteristics](#9-performance-characteristics)
- [Build & Test](#10-build--test)
- [Future Phases](#11-future-phases)

---

## 1. System Overview

This simulator models a **low-latency order matching engine** with a three-stage asymmetric pipeline architecture. It accepts TCP orders from clients, matches them against a central limit order book (CLOB), and reports execution results through an asynchronous telemetry system.

```
┌──────────┐       ┌──────────────┐       ┌──────────────┐       ┌────────────┐
│  Client   │──TCP──│   Ingress    │──Q1──▶│   Matching   │──Q2──▶│  Telemetry │──▶ stdout
│ (Python)  │◀─ACK──│   Thread     │       │   Engine     │       │   Logger   │
└──────────┘       └──────────────┘       └──────────────┘       └────────────┘
                    Thread 1 (Main)        Thread 2 (Spawned)     Thread 3 (Spawned)
```

**Design philosophy:** Every thread has a single, well-defined responsibility. Data flows strictly left-to-right. No thread ever reaches backward in the pipeline.

---

## 2. Thread Topology

| Thread | Identity | Role | Blocks On | Never Touches |
|--------|----------|------|-----------|---------------|
| **Thread 1** | Main thread | Ingress — `select()` network loop | `select()` (1s timeout) | OrderBook, stdout |
| **Thread 2** | `engine_thread_` | Matching Engine — order processing | `condvar` (order queue) | Sockets, stdout |
| **Thread 3** | `logger thread_` | Telemetry Logger — async output | `condvar` (telemetry queue) | Sockets, OrderBook |

### Startup Order (Reverse Pipeline)

```cpp
logger.start();   // Thread 3: consumer of telemetry events — ready first
engine.start();   // Thread 2: consumer of orders — ready second
server.start();   // Thread 1: producer of orders — starts last (blocks main)
```

Consumers start before producers so no events are dropped during initialization.

### Shutdown Order (Forward Pipeline)

```cpp
server.stop();    // Stop accepting connections
engine.stop();    // Drain order queue, join thread
logger.stop();    // Drain telemetry queue, join thread
```

---

## 3. File Architecture

```
HighFrequencyTrading/
├── include/
│   ├── Order.h               # Order & Trade structs, OrderSide/OrderStatus enums
│   ├── OrderBook.h            # CLOB interface (std::map-based price levels)
│   ├── ThreadSafeQueue.h      # MPSC queue (mutex + condvar), template
│   ├── MatchingEngine.h       # Engine thread: queue consumer, book accessor
│   ├── TelemetryLogger.h      # Logger thread: async stdout drain
│   └── TCPServer.h            # Ingress thread: select()-based network loop
├── src/
│   ├── main.cpp               # Pipeline wiring, thread lifecycle
│   ├── OrderBook.cpp          # Price-time priority matching (no I/O)
│   ├── MatchingEngine.cpp     # Hot loop: dequeue → match → telemetry push
│   ├── TCPServer.cpp          # select() loop, inline client handling
│   └── TelemetryLogger.cpp    # Drain loop: dequeue → cout
├── design/
│   └── hft_simulator_deep_dive.md
├── build/                     # Compiled objects and executable
├── Makefile                   # Build configuration
└── send_test_order.py         # Python test client
```

---

## 4. Data Structures

### Order (`include/Order.h`)

```cpp
struct Order {
    long long order_id;             // Assigned by matching engine (monotonic)
    std::string client_order_id;    // From client (e.g., "B001")
    std::string symbol;             // Ticker (e.g., "MSFT")
    OrderSide side;                 // BUY or SELL
    double price;                   // Limit price
    int quantity;                   // Remaining quantity
    int filled_quantity;            // Cumulative fills
    OrderStatus status;             // NEW, PARTIALLY_FILLED, FILLED, CANCELLED
    time_point timestamp;           // Engine receipt time
};
```

> **Memory note:** With GCC/MSVC Small String Optimization (SSO), strings ≤15 characters are stored inline — no heap allocation for typical ticker symbols and client IDs.

### Trade (`include/Order.h`)

```cpp
struct Trade {
    std::string symbol;
    double price;
    int quantity;
    long long buy_order_id;
    long long sell_order_id;
    time_point timestamp;
};
```

### OrderBook (`include/OrderBook.h`)

```cpp
// BUY side: highest price first (aggressive buyers at top)
std::map<double, std::list<Order>, std::greater<double>> buy_orders_;

// SELL side: lowest price first (aggressive sellers at top)
std::map<double, std::list<Order>> sell_orders_;
```

**Price-time priority:** `std::map` sorts by price. Within each price level, `std::list` maintains insertion order (FIFO). First order in at a given price gets filled first.

---

## 5. Inter-Thread Communication

### Queue 1: Order Queue (Ingress → Engine)

```
Type:       ThreadSafeQueue<Order>
Producer:   Ingress thread (TCPServer::handle_client_inline)
Consumer:   Engine thread (MatchingEngine::run)
Mechanism:  std::mutex + std::condition_variable
```

### Queue 2: Telemetry Queue (Engine/Ingress → Logger)

```
Type:       ThreadSafeQueue<TelemetryEvent>
Producers:  Engine thread + Ingress thread (both call logger_.log())
Consumer:   Logger thread (TelemetryLogger::run)
Mechanism:  std::mutex + std::condition_variable
```

### ThreadSafeQueue API

| Method | Thread Safety | Behavior |
|--------|--------------|----------|
| `push(T)` | Lock + notify | Enqueues item, wakes one consumer |
| `wait_and_pop()` | Lock + wait | Blocks until item available or stopped |
| `try_pop()` | Lock only | Non-blocking, returns `std::optional<T>` |
| `stop()` | Lock + notify_all | Signals shutdown, wakes all waiters |

---

## 6. Pipeline Stages — Detailed

### Stage 1: Ingress Thread (`TCPServer::start`)

```
┌─ select() EVENT LOOP ─────────────────────────────────┐
│                                                        │
│  select(server_fd_, timeout=1s)                        │
│       │                                                │
│       ├── timeout → check running_ → loop              │
│       │                                                │
│       └── server_fd_ readable → accept()               │
│               │                                        │
│               ▼                                        │
│       handle_client_inline(client_socket):             │
│         1. Set SO_RCVTIMEO = 200ms                     │
│         2. recv() → raw bytes                          │
│         3. parse_order_message() → Order struct        │
│         4. send("RECEIVED:<id>:SEQ:<n>") → client      │
│         5. matching_engine_.enqueue_order(move(order)) │
│         6. closesocket(client_socket)                  │
│         7. logger_.log("[INGRESS] Queued ...")          │
│                                                        │
└────────────────────────────────────────────────────────┘
```

**Key properties:**
- Single thread handles all connections — no thread creation overhead
- `select()` with timeout enables graceful shutdown via `running_` flag
- `SO_RCVTIMEO` prevents a slow client from stalling the entire ingress
- Client gets `RECEIVED` ACK before matching happens — decoupled latency

### Stage 2: Matching Engine (`MatchingEngine::run`)

```
┌─ HOT LOOP ─────────────────────────────────────────────┐
│                                                         │
│  Order order = order_queue_.wait_and_pop()              │
│       │                                                 │
│       ▼                                                 │
│  auto t_start = high_resolution_clock::now()            │
│  vector<Trade> trades = order_book_.add_order(order)    │
│  auto latency = t_end - t_start                         │
│       │                                                 │
│       ▼                                                 │
│  Format result string via ostringstream (stack-local)   │
│  logger_.log(result)           → push to telemetry Q    │
│  logger_.log(book.snapshot())  → push to telemetry Q    │
│  orders_processed_++           → atomic increment       │
│                                                         │
│  └── loop back to wait_and_pop()                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**Key properties:**
- **ZERO system calls** in the hot path (no `write()`, no `send()`, no `cout`)
- `add_order()` does pure in-memory tree operations
- String formatting via `ostringstream` uses stack-allocated buffers
- `logger_.log()` is just a queue push (~150ns)
- Measured latency: **4,700–69,900 ns** per order

### Stage 3: Telemetry Logger (`TelemetryLogger::run`)

```
┌─ DRAIN LOOP ──────────────────────────────────────┐
│                                                    │
│  TelemetryEvent event = queue_.wait_and_pop()      │
│       │                                            │
│       ▼                                            │
│  std::cout << event.message << std::endl           │
│  events_logged_++                                  │
│                                                    │
│  └── loop back to wait_and_pop()                   │
│                                                    │
│  On shutdown: drain remaining events via try_pop() │
│                                                    │
└────────────────────────────────────────────────────┘
```

**Key properties:**
- Only thread that ever writes to `stdout`
- Flush-on-shutdown ensures no telemetry events are lost
- If `stdout` blocks (slow terminal, pipe buffer full), only this thread stalls — engine and ingress are unaffected

---

## 7. Matching Algorithm

The order book implements **price-time priority matching** (the standard algorithm used by most electronic exchanges):

### BUY order arrives:

```
1. Check sell_orders_ (sorted lowest price first)
2. While order.quantity > 0 AND best_sell_price <= order.price:
   a. Match against oldest order at best sell price level
   b. Trade quantity = min(buy.quantity, sell.quantity)
   c. Record trade, update quantities and fill status
   d. If sell order fully filled → remove from level
   e. If price level empty → erase from map
3. If order has remaining quantity → insert into buy_orders_
```

### SELL order arrives:

```
1. Check buy_orders_ (sorted highest price first)
2. While order.quantity > 0 AND best_buy_price >= order.price:
   a. Match against oldest order at best buy price level
   b. Trade quantity = min(sell.quantity, buy.quantity)
   c. Record trade, update quantities and fill status
   d. If buy order fully filled → remove from level
   e. If price level empty → erase from map
3. If order has remaining quantity → insert into sell_orders_
```

### Complexity:
- **Best case** (no match): O(log n) — single `map::insert`
- **Match at top-of-book**: O(log n) — one `map::begin()` + potential `map::erase`
- **Sweep through multiple levels**: O(k · log n) where k = number of price levels touched

---

## 8. Network Protocol

### Client → Server (Order):
```
SYMBOL,SIDE,PRICE,QUANTITY,CLIENT_ORDER_ID\n
```
Example: `MSFT,BUY,300.00,50,B001`

### Server → Client (Immediate ACK):
```
RECEIVED:<CLIENT_ORDER_ID>:SEQ:<SEQUENCE_NUMBER>\n
```
Example: `RECEIVED:B001:SEQ:1`

### Server → Client (Parse Error):
```
ERR:<description>\n
```
Example: `ERR:Invalid format. Expected 5 fields, got 4`

> **Note:** The `SEQ` number is a monotonic counter assigned by the ingress thread. It provides a unique identifier for tracking order lifecycle through the telemetry output. Execution reports (fills, trades) are logged server-side via the telemetry pipeline and are not sent back to the client on the same TCP connection — this is intentional, mirroring real exchange architectures where execution reports arrive on separate sessions.

---

## 9. Performance Characteristics

| Metric | Value | Where Measured |
|--------|-------|----------------|
| Order matching latency | 4,700–69,900 ns | `MatchingEngine::run()` `high_resolution_clock` |
| Queue handoff latency | ~150 ns | mutex lock + push + unlock + notify |
| Thread count | 3 (static) | Never changes at runtime |
| Heap allocs per order | ~0 (SSO strings) | Order struct fits in stack/inline |
| System calls on hot path | 0 | Engine thread does no I/O |
| Max concurrent connections | FD_SETSIZE (64 on Windows) | `select()` limit |

---

## 10. Build & Test

### Build
```bash
# Windows (MinGW/MSYS2)
mingw32-make

# Manual
g++ -std=c++17 -Wall -Wextra -Iinclude -pthread \
    src/main.cpp src/OrderBook.cpp src/TCPServer.cpp \
    src/MatchingEngine.cpp src/TelemetryLogger.cpp \
    -o build/hft_core_phase1 -lws2_32
```

### Run
```bash
# Terminal 1: Start server
./build/hft_core_phase1

# Terminal 2: Run test client
python send_test_order.py
```

### Expected Server Output
```
========================================
 HFT Core — Phase A: Asymmetric Pipeline
========================================
[TELEMETRY] Logger thread started.
[ENGINE] Matching engine hot loop started.
[INGRESS] select()-based ingress loop listening on port 8080
[INGRESS] Connection from 127.0.0.1:59738
[INGRESS] Queued order SEQ:1 (MSFT,BUY,300.00,50,B001)
[ENGINE] Order 1 (Client:B001 BUY 50@300.00) matched in 20700 ns | Status: NEW
--- Order Book Snapshot ---
SELL Orders: (empty)
BUY Orders:
  Price 300.00: 50 (B001, ID:1) |
--------------------------
```

---

## 11. Future Phases

| Phase | Feature | Impact |
|-------|---------|--------|
| **B** | Lock-free SPSC queue (ring buffer) | Eliminate mutex on hot path |
| **C** | Memory pool for Order/Trade objects | Eliminate heap allocation |
| **D** | CPU core pinning (`SetThreadAffinityMask`) | Eliminate scheduler jitter |
| **E** | Fixed-size strings (`char[16]`) in Order | Eliminate SSO branching |
| **F** | Binary protocol (FIX/SBE) | Eliminate string parsing overhead |
| **G** | Kernel bypass (DPDK/io_uring) | Eliminate syscall overhead |
