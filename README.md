# High-Frequency Limit Order Book & Matching Engine (C++17)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20Make%20%7C%20MSVC-brightgreen.svg)]()
[![Performance](https://img.shields.io/badge/Throughput-2.35M%2B%20orders%2Fsec-blue.svg)]()
[![Latency](https://img.shields.io/badge/p50%20Latency-300%20ns-orange.svg)]()
[![Tests](https://img.shields.io/badge/Tests-50%2C019%20Checks%20Passed-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-blue.svg)]()

> **Domain:** Low-Latency Systems / Quantitative Engineering & Matching Engine Architecture  
> **Key Capabilities Demonstrated:** Deterministic price-time priority matching, strict $O(1)$ order cancellation via iterator locators, integer tick pricing without floating-point hazards, comprehensive invariant fuzzing, and nanosecond-scale benchmarking.

---

## 📌 Executive Summary

In high-frequency and algorithmic trading environments, the matching engine is the core deterministic state machine. Latency jitter, memory allocation overhead, and non-deterministic order cancellations degrade trading alpha and introduce execution slippage.

This project implements a single-threaded limit order book in modern **C++17** engineered around core quantitative systems principles:
1. **Price-Time Priority (FIFO):** Better prices execute first; at equal prices, earlier arrivals execute first.
2. **Deterministic $O(1)$ Cancellations:** Direct order index mapping to doubly-linked list iterators enables instant cancellation without iterating price levels.
3. **Maker-Price Execution:** Trades execute strictly at the price of the resting (maker) order, rewarding liquidity providers.
4. **Integer Tick Representation:** Eliminates IEEE-754 floating-point rounding errors and non-associative comparison issues by enforcing 64-bit integer price ticks.
5. **Zero-Allocation Execution Path on Matches:** Fast in-place level traversal with configurable callback sink for trade dissemination.

---

## 🏛 Data Structure & Complexity Analysis

```mermaid
flowchart LR
    subgraph Order Index: O(1) Lookup
        Idx[unordered_map<OrderId, Locator>]
    end

    subgraph Bids (std::greater)
        B1[Price: 101] --> B2[Price: 100] --> B3[Price: 99]
    end

    subgraph Asks (std::less)
        A1[Price: 103] --> A2[Price: 104] --> A3[Price: 105]
    end

    subgraph Price Level Queue: FIFO Time Priority
        B1 --> L1["Order #2 (qty: 5)"] --> L2["Order #7 (qty: 12)"]
    end

    Idx -.->|Direct Iterator| L1
```

### Algorithmic Complexity
| Operation | Data Structure | Time Complexity | Details |
|---|---|---|---|
| **Best Bid / Best Ask** | `std::map::begin()` | $\mathcal{O}(1)$ | Pre-sorted tree roots expose best market prices instantly. |
| **Insert New Price Level** | `std::map::operator[]` | $\mathcal{O}(\log P)$ | $P$ is the number of active price levels. |
| **Append Passive Limit Order** | `std::list::push_back` | $\mathcal{O}(1)$ | Maintains arrival order at the specific price level. |
| **Match Aggressive Order** | Level traversal | $\mathcal{O}(K)$ | Proportional to number of filled resting orders $K$. |
| **Cancel Order by ID** | `std::unordered_map` + `list::erase` | $\mathcal{O}(1)$ | Hash lookup returns `list::iterator`; in-place pointer unlinking. |

---

## 💡 Core Engineering Highlights

### 1. Constant-Time $O(1)$ Order Cancellation
A common bottleneck in naive order books is searching across levels to find an order when a cancel request arrives. 

Here, every resting order is tracked in a hash index using a `Locator` struct:
```cpp
struct Locator {
    Side side;
    Price price;
    Level::iterator it; // Direct iterator to node in std::list<Order>
};
std::unordered_map<OrderId, Locator> index_;
```
When `cancel(id)` is called:
1. `index_.find(id)` looks up the order's metadata in $\mathcal{O}(1)$ average time.
2. The price level list directly erases the node via `level.erase(loc.it)` in $\mathcal{O}(1)$ pointer operations without any search loop.
3. If the price level becomes empty, it is purged from the price tree.

### 2. Market vs Limit Matching Mechanics
- **Limit Orders (`add_limit`):**
  - Taker order matches against opposing book if price crosses the spread ($\text{bid} \ge \text{best\_ask}$ or $\text{ask} \le \text{best\_bid}$).
  - Trades execute at the resting maker's price.
  - Any remaining unfilled quantity is enqueued at the back of its designated price level.
- **Market Orders (`add_market`):**
  - Sweeps consecutive price levels until fully filled or until opposite liquidity is completely exhausted.
  - Any unfilled remainder is discarded (IOC/fill-or-kill style exhaustion).

### 3. Downstream Event Pipeline
The engine accepts a functional callback hook:
```cpp
using TradeSink = std::function<void(const Trade&)>;
```
Whenever a trade matches, a lightweight `Trade{taker_id, maker_id, price, qty}` struct is dispatched synchronously to the consumer (e.g., market data feeds, risk monitors, or strategy simulators) without dynamic heap allocations.

---

## 🛠 Tech Stack & Portability

- **Language:** ISO C++17
- **Standard Library:** Containers (`<map>`, `<list>`, `<unordered_map>`, `<vector>`), time utilities (`<chrono>`), pseudo-random generation (`<random>`).
- **Compilers Supported:**
  - GCC 7+ (Linux / WSL / MinGW)
  - Clang 6+ (macOS / Linux)
  - MSVC 2017+ (Visual Studio 2019/2022/2026 Build Tools)
- **Build Systems:** Makefile, CMake 3.14+, and native Windows `build.bat`.

---

## 🚀 Building & Running

### Option A: Using CMake (Cross-Platform)
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Run tests
ctest --output-on-failure
# Or directly:
./test_orderbook     # (or .\Release\test_orderbook.exe on Windows)

# Run benchmark
./bench 2000000      # (or .\Release\bench.exe 2000000 on Windows)
```

### Option B: Using Makefile (Linux / macOS / MinGW)
```bash
make test            # Compiles and executes test suite
make bench           # Compiles and runs 2,000,000 order benchmark
make clean
```

### Option C: Using MSVC Command Line (Windows)
```cmd
build.bat
build\test_orderbook.exe
build\bench.exe 2000000
```

---

## 🧪 Verification & Invariant Fuzzing

The test suite (`tests/test_orderbook.cpp`) verifies every operational mode followed by a 50,000-order randomized fuzzing run:

```bash
build/test_orderbook
```
**Output:**
```
All 50019 checks passed
```

### Verified Test Suites
1. `test_rest_and_best_prices`: Verifies correct ordering and best bid/ask tracking.
2. `test_price_priority`: Better price matches first regardless of arrival time.
3. `test_time_priority_within_level`: First-in-first-out execution when prices are identical.
4. `test_partial_fill_rests_remainder`: Ensures partial matches update resting quantity without losing queue position.
5. `test_trade_executes_at_maker_price`: Taker with price limit 110 matching maker at 100 executes at 100.
6. `test_market_order_sweeps_levels`: Sweeps across multiple ladder depths.
7. `test_market_order_unfilled_remainder_discarded`: Validates non-resting market behavior.
8. `test_cancel`: Validates $O(1)$ cancellation and level pruning.
9. `test_duplicate_id_rejected`: Enforces unique order IDs.
10. `test_random_flow_invariants` (50,000 randomized orders):
    - **Never Crossed Book Invariant:** $\text{best\_bid} < \text{best\_ask}$ at all times.
    - **Conservation of Quantity Invariant:** $\text{Resting} + 2 \times \text{Traded} \le \text{Submitted}$ holds across every step.

---

## ⚡ Performance Benchmark

The benchmark (`src/bench.cpp`) generates 2,000,000 synthetic orders modeling realistic market conditions:
- **60% Passive Limit Orders** clustered around mid-price.
- **20% Aggressive Crossing Orders** triggering matches.
- **20% Order Cancellations** removing live orders.

```bash
build/bench 2000000
```

### Empirical Benchmark Results
```
orders processed : 2,000,000
throughput       : 2.35 - 2.50 Million orders / sec
latency p50      : 300 ns
latency p99      : 900 ns
latency p99.9    : 1,800 ns
resting orders   : 530,571
```

*Note: Latency is sampled per invocation using `std::chrono::steady_clock`, which incurs ~20-30ns timing overhead.*

---

## 🔬 Systems Extensions & Quant Discussion Points

When discussing high-throughput matching engines in quantitative interviews:
1. **Intrusive Data Structures & Memory Pools:** Replacing standard `std::list` nodes with a pre-allocated slab allocator or contiguous intrusive doubly-linked list avoids node allocation jitter and cache misses.
2. **Flat Price Ladder:** For liquid instruments with narrow price bands, replacing `std::map` with a contiguous array/ring buffer indexed by `(price - min_price) / tick_size` yields true $O(1)$ level lookup with optimal L1/L2 cache locality.
3. **Advanced Order Types:** Adding Immediate-Or-Cancel (IOC), Fill-Or-Kill (FOK), and Self-Trade Prevention (STP).
4. **Market Data Feed Handler:** Parsing ITCH 5.0 / OUCH protocols or simple binary UDP packets directly into book events.

---

## 📂 Project Structure

```
orderbook-engine/
├── include/
│   └── orderbook.hpp       # OrderBook, Order, Trade definitions & interfaces
├── src/
│   ├── orderbook.cpp       # Matching engine implementation & cancel logic
│   └── bench.cpp           # 2M order high-frequency benchmark harness
├── tests/
│   └── test_orderbook.cpp  # Unit tests & 50,000-order invariant fuzzer
├── CMakeLists.txt          # Modern cross-platform CMake configuration
├── Makefile                # GCC/Clang build script
├── build.bat               # Windows MSVC/GCC automatic build script
├── .gitignore              # Ignores build artifacts (.obj, .exe, .o)
└── README.md
```

---

## 🛡 License
This project is licensed under the MIT License.
