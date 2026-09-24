#include "micromatch/book.hpp"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace micromatch {
namespace {
Response reject(Id id, std::string code) {
    Response result;
    result.ok = false; result.id = id; result.code = std::move(code);
    return result;
}
}
OrderBook::OrderBook(std::size_t max_active, std::size_t max_seen)
    : max_active_(max_active), max_seen_(max_seen) {
    if (!max_active || !max_seen) throw std::invalid_argument("Capacity must be positive");
    active_.reserve(std::min<std::size_t>(max_active, 4096));
    seen_.reserve(std::min<std::size_t>(max_seen, 4096));
}

template<class Levels>
void OrderBook::match(Add const& input, Qty& left, Levels& opposite, Response& result) {
    while (left > 0 && !opposite.empty()) {
        auto best = opposite.begin();
        if (!input.market && (input.side == Side::Buy ? best->first > input.price : best->first < input.price)) break;
        auto& orders = best->second;
        auto& resting = orders.front();
        const Qty executed = std::min(left, resting.quantity);
        // Allocate the report before changing this fill's quantities.
        result.trades.push_back({resting.id, input.id, resting.price, executed});
        left -= executed;
        resting.quantity -= executed;
        if (resting.quantity == 0) {
            active_.erase(resting.id);
            orders.pop_front();
        }
        if (orders.empty()) opposite.erase(best);
    }
}

Response OrderBook::add(Add const& input) {
    if (input.id == 0) return reject(input.id, "invalid_id");
    if (input.side != Side::Buy && input.side != Side::Sell) return reject(input.id, "invalid_side");
    if (input.quantity <= 0 || input.quantity > 1000000) return reject(input.id, "invalid_quantity");
    if (!input.market && (input.price <= 0 || input.price > 1000000000)) return reject(input.id, "invalid_price");
    if (seen_.contains(input.id)) return reject(input.id, "duplicate_id");
    if (seen_.size() >= max_seen_ || active_.size() >= max_active_) return reject(input.id, "capacity_limit");
    seen_.insert(input.id);
    Response result;
    result.id = input.id;
    Qty left = input.quantity;
    if (input.side == Side::Buy) match(input, left, asks_, result);
    else match(input, left, bids_, result);
    if (left > 0 && !input.market) {
        auto& orders = input.side == Side::Buy ? bids_[input.price] : asks_[input.price];
        orders.push_back({input.id, input.side, input.price, left});
        active_.emplace(input.id, Locator{input.side, input.price, std::prev(orders.end())});
    }
    result.remaining = left; // Market remainder is expired, not rested.
    if (input.market && left > 0) result.code = "market_remainder_expired";
    return result;
}
Response OrderBook::cancel(Id id) {
    auto found = active_.find(id);
    if (found == active_.end()) return reject(id, "unknown_order");
    auto const locator = found->second;
    Response result;
    result.id = id; result.code = "cancelled"; result.remaining = locator.position->quantity;
    auto erase = [&](auto& levels) {
        auto level = levels.find(locator.price);
        level->second.erase(locator.position);
        if (level->second.empty()) levels.erase(level);
    };
    if (locator.side == Side::Buy) erase(bids_); else erase(asks_);
    active_.erase(found);
    return result;
}
Response OrderBook::depth(std::size_t count) const {
    Response result; result.code = "book";
    count = std::min<std::size_t>(count, 100);
    auto gather = [count](auto const& source, auto& target) {
        for (auto const& [price, orders] : source) {
            if (target.size() == count) break;
            Qty total = 0;
            for (auto const& order : orders) total += order.quantity;
            target.push_back({price, total, orders.size()});
        }
    };
    gather(bids_, result.bids); gather(asks_, result.asks);
    return result;
}
Response OrderBook::execute(Command const& command) {
    return std::visit([this](auto const& value) -> Response {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Add>) return add(value);
        else if constexpr (std::is_same_v<T, Cancel>) return cancel(value.id);
        else return depth(value.levels);
    }, command);
}
bool OrderBook::invariant() const {
    if (!bids_.empty() && !asks_.empty() && bids_.begin()->first >= asks_.begin()->first) return false;
    std::size_t count = 0;
    auto check = [&](auto const& levels, Side side) {
        for (auto const& [price, orders] : levels) {
            if (orders.empty()) return false;
            for (auto it = orders.begin(); it != orders.end(); ++it) {
                auto found = active_.find(it->id);
                if (it->side != side || it->price != price || it->quantity <= 0 || !seen_.contains(it->id) || found == active_.end()) return false;
                if (found->second.side != side || found->second.price != price || &*found->second.position != &*it) return false;
                ++count;
            }
        }
        return true;
    };
    return check(bids_, Side::Buy) && check(asks_, Side::Sell) && count == active_.size();
}
}
