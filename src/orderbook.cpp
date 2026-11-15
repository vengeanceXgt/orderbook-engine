#include "orderbook.hpp"

namespace ob {

template <typename BookSide, typename Crosses>
Quantity OrderBook::match(OrderId taker, Quantity qty, BookSide& book, Crosses crosses) {
    Quantity filled = 0;
    while (qty > 0 && !book.empty()) {
        auto level_it = book.begin();
        if (!crosses(level_it->first)) break;
        Level& level = level_it->second;

        while (qty > 0 && !level.empty()) {
            Order& maker = level.front();
            const Quantity traded = std::min(qty, maker.qty);
            if (sink_) sink_(Trade{taker, maker.id, level_it->first, traded});
            maker.qty -= traded;
            qty -= traded;
            filled += traded;
            if (maker.qty == 0) {
                index_.erase(maker.id);
                level.pop_front();
            }
        }
        if (level.empty()) book.erase(level_it);
    }
    return filled;
}

void OrderBook::rest(const Order& o) {
    if (o.side == Side::Buy) {
        auto& level = bids_[o.price];
        level.push_back(o);
        index_[o.id] = Locator{o.side, o.price, std::prev(level.end())};
    } else {
        auto& level = asks_[o.price];
        level.push_back(o);
        index_[o.id] = Locator{o.side, o.price, std::prev(level.end())};
    }
}

Quantity OrderBook::add_limit(OrderId id, Side side, Price price, Quantity qty) {
    if (qty <= 0 || index_.count(id)) return 0;
    Quantity filled;
    if (side == Side::Buy) {
        filled = match(id, qty, asks_, [price](Price ask) { return ask <= price; });
    } else {
        filled = match(id, qty, bids_, [price](Price bid) { return bid >= price; });
    }
    const Quantity remaining = qty - filled;
    if (remaining > 0) rest(Order{id, side, price, remaining});
    return filled;
}

Quantity OrderBook::add_market(OrderId id, Side side, Quantity qty) {
    if (qty <= 0) return 0;
    if (side == Side::Buy) return match(id, qty, asks_, [](Price) { return true; });
    return match(id, qty, bids_, [](Price) { return true; });
}

bool OrderBook::cancel(OrderId id) {
    auto found = index_.find(id);
    if (found == index_.end()) return false;
    const Locator loc = found->second;
    if (loc.side == Side::Buy) {
        auto lvl = bids_.find(loc.price);
        lvl->second.erase(loc.it);
        if (lvl->second.empty()) bids_.erase(lvl);
    } else {
        auto lvl = asks_.find(loc.price);
        lvl->second.erase(loc.it);
        if (lvl->second.empty()) asks_.erase(lvl);
    }
    index_.erase(found);
    return true;
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

Quantity OrderBook::depth_at(Side side, Price price) const {
    Quantity total = 0;
    auto sum = [&](const Level& level) {
        for (const auto& o : level) total += o.qty;
    };
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        if (it != bids_.end()) sum(it->second);
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end()) sum(it->second);
    }
    return total;
}

}  // namespace ob
