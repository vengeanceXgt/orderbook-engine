// Throughput and latency benchmark on a synthetic order flow.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "orderbook.hpp"

using Clock = std::chrono::steady_clock;

int main(int argc, char** argv) {
    const std::size_t n = argc > 1 ? std::stoull(argv[1]) : 2'000'000;
    std::mt19937_64 rng(7);
    ob::OrderBook book;
    std::vector<ob::OrderId> live;
    live.reserve(n);
    std::vector<double> lat;
    lat.reserve(n);

    const auto t0 = Clock::now();
    for (ob::OrderId id = 1; id <= n; ++id) {
        const int action = rng() % 10;
        const auto s0 = Clock::now();
        if (action < 6) {  // limit order near the mid price
            auto side = (rng() & 1) ? ob::Side::Buy : ob::Side::Sell;
            ob::Price mid = 100000;
            ob::Price offset = static_cast<ob::Price>(rng() % 50);
            ob::Price price = side == ob::Side::Buy ? mid - offset : mid + offset;
            book.add_limit(id, side, price, 1 + rng() % 100);
            live.push_back(id);
        } else if (action < 8) {  // aggressive order that crosses the spread
            auto side = (rng() & 1) ? ob::Side::Buy : ob::Side::Sell;
            ob::Price price = side == ob::Side::Buy ? 100060 : 99940;
            book.add_limit(id, side, price, 1 + rng() % 100);
        } else if (!live.empty()) {  // cancel
            std::size_t i = rng() % live.size();
            book.cancel(live[i]);
            live[i] = live.back();
            live.pop_back();
        }
        const auto s1 = Clock::now();
        lat.push_back(std::chrono::duration<double, std::nano>(s1 - s0).count());
    }
    const double secs = std::chrono::duration<double>(Clock::now() - t0).count();

    std::sort(lat.begin(), lat.end());
    auto pct = [&](double p) { return lat[static_cast<std::size_t>(p * (lat.size() - 1))]; };
    std::printf("orders processed : %zu\n", n);
    std::printf("throughput       : %.2f M orders/sec\n", n / secs / 1e6);
    std::printf("latency p50      : %.0f ns\n", pct(0.50));
    std::printf("latency p99      : %.0f ns\n", pct(0.99));
    std::printf("latency p99.9    : %.0f ns\n", pct(0.999));
    std::printf("resting orders   : %zu\n", book.resting_orders());
}
