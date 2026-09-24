#include "micromatch/book.hpp"
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string_view>
namespace micromatch {
namespace {
template<class T> T number(std::string const& text) {
    T value{};
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size()) throw std::invalid_argument("invalid_number");
    return value;
}
Side side(std::string const& value) {
    if (value == "BUY") return Side::Buy;
    if (value == "SELL") return Side::Sell;
    throw std::invalid_argument("invalid_side");
}
}
Command parse(std::string const& line) {
    if (line.size() > 256) throw std::invalid_argument("line_too_long");
    std::istringstream stream(line);
    std::vector<std::string> parts;
    for (std::string token; stream >> token;) parts.push_back(token);
    if (parts.size() == 5 && parts[0] == "LIMIT") return Add{number<Id>(parts[1]), side(parts[2]), number<Price>(parts[3]), number<Qty>(parts[4]), false};
    if (parts.size() == 4 && parts[0] == "MARKET") return Add{number<Id>(parts[1]), side(parts[2]), 0, number<Qty>(parts[3]), true};
    if (parts.size() == 2 && parts[0] == "CANCEL") return Cancel{number<Id>(parts[1])};
    if (parts.size() == 1 && parts[0] == "BOOK") return Depth{};
    if (parts.size() == 2 && parts[0] == "BOOK") {
        auto n = number<std::size_t>(parts[1]);
        if (n < 1 || n > 100) throw std::invalid_argument("invalid_depth");
        return Depth{n};
    }
    throw std::invalid_argument("invalid_command");
}
std::string json(Response const& r) {
    // All codes are fixed internal strings, never reflected user input.
    std::ostringstream out;
    out << "{\"ok\":" << (r.ok ? "true" : "false") << ",\"code\":\"" << r.code << "\",\"id\":" << r.id << ",\"remaining\":" << r.remaining << ",\"trades\":[";
    bool first = true;
    for (auto const& t : r.trades) {
        if (!first) out << ',';
        first = false;
        out << "{\"maker\":" << t.maker << ",\"taker\":" << t.taker << ",\"price\":" << t.price << ",\"quantity\":" << t.quantity << '}';
    }
    out << ']';
    auto levels = [&](char const* name, auto const& values) {
        out << ",\"" << name << "\":["; bool initial = true;
        for (auto const& l : values) {
            if (!initial) out << ',';
            initial = false;
            out << "{\"price\":" << l.price << ",\"quantity\":" << l.quantity << ",\"orders\":" << l.orders << '}';
        }
        out << ']';
    };
    levels("bids", r.bids); levels("asks", r.asks); out << '}'; return out.str();
}
}
