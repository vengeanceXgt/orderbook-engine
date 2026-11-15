// Limit order book with price-time priority matching.
//
// Design:
//  - Prices are integer ticks (no floating point in the matching path).
//  - Each side is a std::map<price, std::list<Order>>: O(log P) to find/insert a
//    price level, O(1) to append at the back (time priority) or pop the front.
//  - An unordered_map<order id, {side, price, list iterator}> gives O(1) cancel.
#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ob {

using OrderId = std::uint64_t;
using Price = std::int64_t;     // integer ticks
using Quantity = std::int64_t;

enum class Side : std::uint8_t { Buy, Sell };

struct Order {
    OrderId id;
    Side side;
    Price price;       // ignored for market orders
    Quantity qty;      // remaining quantity
};

struct Trade {
    OrderId taker_id;
    OrderId maker_id;
    Price price;       // price of the resting (maker) order
    Quantity qty;
};

class OrderBook {
public:
    using TradeSink = std::function<void(const Trade&)>;

    explicit OrderBook(TradeSink sink = nullptr) : sink_(std::move(sink)) {}

    // Submit a limit order. Matches against the opposite side first; any
    // remainder rests on the book. Returns the quantity that was filled.
    Quantity add_limit(OrderId id, Side side, Price price, Quantity qty);

    // Submit a market order. Fills against the best available prices; any
    // unfilled remainder is discarded. Returns the quantity filled.
    Quantity add_market(OrderId id, Side side, Quantity qty);

    // Cancel a resting order. Returns false if the id is unknown.
    bool cancel(OrderId id);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    Quantity depth_at(Side side, Price price) const;
    std::size_t resting_orders() const { return index_.size(); }

private:
    using Level = std::list<Order>;
    struct Locator {
        Side side;
        Price price;
        Level::iterator it;
    };

    template <typename BookSide, typename Crosses>
    Quantity match(OrderId taker, Quantity qty, BookSide& book, Crosses crosses);

    void rest(const Order& o);

    std::map<Price, Level, std::greater<Price>> bids_;  // best bid first
    std::map<Price, Level, std::less<Price>> asks_;     // best ask first
    std::unordered_map<OrderId, Locator> index_;
    TradeSink sink_;
};

}  // namespace ob
