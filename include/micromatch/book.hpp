#pragma once
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace micromatch {
using Id = std::uint64_t;
using Price = std::int64_t; // Integer ticks, never floating-point money.
using Qty = std::int64_t;
enum class Side { Buy, Sell };
struct Add { Id id; Side side; Price price; Qty quantity; bool market = false; };
struct Cancel { Id id; };
struct Depth { std::size_t levels = 10; };
using Command = std::variant<Add, Cancel, Depth>;
struct Trade { Id maker; Id taker; Price price; Qty quantity; };
struct Level { Price price; Qty quantity; std::size_t orders; };
struct Response {
    bool ok = true;
    std::string code = "accepted";
    Id id = 0;
    Qty remaining = 0;
    std::vector<Trade> trades;
    std::vector<Level> bids, asks;
};
struct Order { Id id; Side side; Price price; Qty quantity; };

// Noncopyable: locators contain iterators into lists owned by THIS book.
// Only the engine worker may mutate this object when used concurrently.
class OrderBook {
    using Orders = std::list<Order>;
    using Bids = std::map<Price, Orders, std::greater<Price>>;
    using Asks = std::map<Price, Orders>;
    struct Locator { Side side; Price price; Orders::iterator position; };
    Bids bids_;
    Asks asks_;
    std::unordered_map<Id, Locator> active_;
    std::unordered_set<Id> seen_;
    std::size_t max_active_;
    std::size_t max_seen_;
    template<class Levels> void match(Add const& input, Qty& left, Levels& opposite, Response& result);
public:
    explicit OrderBook(std::size_t max_active = 100000, std::size_t max_seen = 2000000);
    OrderBook(OrderBook const&) = delete;
    OrderBook& operator=(OrderBook const&) = delete;
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;
    Response add(Add const& input);
    Response cancel(Id id);
    Response depth(std::size_t levels = 10) const;
    Response execute(Command const& command);
    std::size_t active_count() const noexcept { return active_.size(); }
    bool invariant() const;
};
std::string json(Response const& result);
Command parse(std::string const& line);
}
