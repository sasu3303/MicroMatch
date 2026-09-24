#include "micromatch/engine.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
using namespace micromatch;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string("Check failed: ") + #x + " at line " + std::to_string(__LINE__)); } while(false)
namespace {
int count = 0;
void test(char const* name, std::function<void()> fn) { fn(); ++count; std::cout << "PASS " << name << '\n'; }
Add buy(Id id, Price price, Qty qty) { return {id, Side::Buy, price, qty}; }
Add sell(Id id, Price price, Qty qty) { return {id, Side::Sell, price, qty}; }
// Independent O(N) reference: scan a vector for best price, oldest vector index wins ties.
struct Reference {
    std::vector<Order> orders;
    Response add(Add const& in) {
        Response r; r.id = in.id; Qty left = in.quantity;
        while (left > 0) {
            auto best = orders.end();
            for (auto it = orders.begin(); it != orders.end(); ++it) {
                if (it->side == in.side) continue;
                if (!in.market && (in.side == Side::Buy ? it->price > in.price : it->price < in.price)) continue;
                if (best == orders.end() || (in.side == Side::Buy ? it->price < best->price : it->price > best->price)) best = it;
            }
            if (best == orders.end()) break;
            Qty qty = std::min(left, best->quantity);
            r.trades.push_back({best->id,in.id,best->price,qty});
            left -= qty; best->quantity -= qty;
            if (best->quantity == 0) orders.erase(best);
        }
        if (left && !in.market) orders.push_back({in.id,in.side,in.price,left});
        r.remaining = left;
        if (left && in.market) r.code = "market_remainder_expired";
        return r;
    }
    Response cancel(Id id) {
        Response r; r.id = id;
        auto it = std::find_if(orders.begin(),orders.end(),[id](auto const& o){return o.id==id;});
        if (it == orders.end()) {r.ok=false;r.code="unknown_order";}
        else {r.code="cancelled";r.remaining=it->quantity;orders.erase(it);}
        return r;
    }
    Response depth() const {
        Response r; r.code="book";
        std::map<Price,Level,std::greater<Price>> bids;
        std::map<Price,Level> asks;
        for(auto const& o:orders) {
            auto& l=o.side==Side::Buy?bids[o.price]:asks[o.price];
            l.price=o.price;l.quantity+=o.quantity;++l.orders;
        }
        for(auto const& [p,l]:bids) { (void)p; r.bids.push_back(l); }
        for(auto const& [p,l]:asks) { (void)p; r.asks.push_back(l); }
        return r;
    }
};
}
int main() {
 try {
 test("empty book",[]{ OrderBook b; CHECK(b.depth().bids.empty()); CHECK(b.invariant()); });
 test("non-crossing limits",[]{OrderBook b;b.add(buy(1,99,4));b.add(sell(2,101,7));CHECK(b.active_count()==2);CHECK(b.depth().asks[0].quantity==7);CHECK(b.invariant());});
 test("maker price and full fill",[]{OrderBook b;b.add(sell(1,100,10));auto r=b.add(buy(2,105,10));CHECK(r.trades.size()==1);CHECK(r.trades[0].price==100);CHECK(r.remaining==0);CHECK(b.active_count()==0);CHECK(b.invariant());});
 test("FIFO within price",[]{OrderBook b;b.add(sell(1,100,10));b.add(sell(2,100,10));auto r=b.add(buy(3,100,15));CHECK(r.trades[0].maker==1);CHECK(r.trades[1].maker==2);CHECK(b.depth().asks[0].quantity==5);CHECK(b.invariant());});
 test("best price before arrival",[]{OrderBook b;b.add(sell(1,105,10));b.add(sell(2,100,10));auto r=b.add(buy(3,110,15));CHECK(r.trades[0].maker==2);CHECK(r.trades[1].maker==1);CHECK(b.invariant());});
 test("sell crosses highest bid",[]{OrderBook b;b.add(buy(1,99,4));b.add(buy(2,101,4));auto r=b.add(sell(3,98,6));CHECK(r.trades[0].price==101);CHECK(r.trades[1].price==99);CHECK(b.invariant());});
 test("incoming remainder rests",[]{OrderBook b;b.add(sell(1,100,3));auto r=b.add(buy(2,101,7));CHECK(r.remaining==4);CHECK(b.depth().bids[0].price==101);CHECK(b.depth().bids[0].quantity==4);CHECK(b.invariant());});
 test("market remainder expires",[]{OrderBook b;b.add(sell(1,100,3));auto r=b.add({2,Side::Buy,0,8,true});CHECK(r.remaining==5);CHECK(r.code=="market_remainder_expired");CHECK(b.active_count()==0);});
 test("empty market expires",[]{OrderBook b;auto r=b.add({1,Side::Sell,0,5,true});CHECK(r.trades.empty());CHECK(r.remaining==5);CHECK(b.active_count()==0);});
 test("cancel middle preserves FIFO",[]{OrderBook b;for(Id id=1;id<=3;++id)b.add(sell(id,100,2));CHECK(b.cancel(2).ok);auto r=b.add(buy(4,100,4));CHECK(r.trades[0].maker==1);CHECK(r.trades[1].maker==3);CHECK(b.invariant());});
 test("cancel missing and filled",[]{OrderBook b;CHECK(!b.cancel(42).ok);b.add(buy(1,100,2));b.add(sell(2,100,2));CHECK(!b.cancel(1).ok);});
 test("duplicate IDs rejected for session",[]{OrderBook b;b.add(buy(1,100,2));CHECK(!b.add(buy(1,100,2)).ok);b.cancel(1);CHECK(!b.add(sell(1,100,2)).ok);});
 test("invalid inputs do not consume ID",[]{OrderBook b;CHECK(!b.add(buy(1,0,2)).ok);CHECK(!b.add(buy(1,100,0)).ok);CHECK(!b.add(buy(1,100,-2)).ok);CHECK(!b.add(buy(0,100,2)).ok);CHECK(!b.add(buy(1,100,1000001)).ok);CHECK(b.add(buy(1,100,2)).ok);});
 test("capacity rejection",[]{OrderBook b(1,2);b.add(buy(1,100,2));CHECK(b.add(buy(2,99,2)).code=="capacity_limit");b.cancel(1);CHECK(b.add(buy(2,99,2)).ok);b.cancel(2);CHECK(b.add(buy(3,99,2)).code=="capacity_limit");});
 test("depth aggregation and ordering",[]{OrderBook b;b.add(buy(1,100,2));b.add(buy(2,100,3));b.add(buy(3,99,4));auto d=b.depth(1);CHECK(d.bids.size()==1);CHECK(d.bids[0].quantity==5);CHECK(d.bids[0].orders==2);});
 test("strict parser",[]{for(auto const& line:{"LIMIT 1 BUY 1.5 4","CANCEL -1","BOOK 0","BOOK 101","CANCEL 18446744073709551616","LIMIT 1 BAD 1 2","LIMIT 1 BUY 1 2 extra",""}){bool threw=false;try{parse(line);}catch(std::invalid_argument const&){threw=true;}CHECK(threw);}CHECK(std::get<Add>(parse("LIMIT 7 BUY 100 2")).id==7);bool threw=false;try{parse(std::string(257,'x'));}catch(std::invalid_argument const&){threw=true;}CHECK(threw);});
 test("randomized reference equivalence 10000 commands",[]{
    OrderBook b;Reference reference;std::mt19937 rng(2027);Id next=1;
    for(int i=0;i<10000;++i){
        Response actual,expected;
        if(i%5==0&&next>1){Id id=1+static_cast<Id>(rng())%(next-1);actual=b.cancel(id);expected=reference.cancel(id);}
        else{Add a{next++,rng()%2?Side::Buy:Side::Sell,95+static_cast<Price>(rng()%11),1+static_cast<Qty>(rng()%30),rng()%9==0};actual=b.add(a);expected=reference.add(a);}
        CHECK(json(actual)==json(expected));CHECK(json(b.depth(100))==json(reference.depth()));CHECK(b.invariant());
    }
 });
 test("queue drains after close",[]{BoundedQueue<int> q(2);CHECK(q.push(1));CHECK(q.push(2));q.close();CHECK(!q.push(3));CHECK(q.pop()==1);CHECK(q.pop()==2);CHECK(!q.pop());});
 test("blocked consumer wakes on close",[]{BoundedQueue<int> q(1);auto f=std::async(std::launch::async,[&]{return q.pop();});q.close();CHECK(f.wait_for(std::chrono::seconds(2))==std::future_status::ready);CHECK(!f.get());});
 test("blocked producer wakes on close",[]{BoundedQueue<int> q(1);q.push(1);auto f=std::async(std::launch::async,[&]{return q.push(2);});CHECK(f.wait_for(std::chrono::milliseconds(20))==std::future_status::timeout);q.close();CHECK(!f.get());CHECK(q.pop()==1);});
 test("zero queue capacity rejected",[]{bool threw=false;try{BoundedQueue<int> q(0);}catch(std::invalid_argument const&){threw=true;}CHECK(threw);});
 test("concurrent engine producers 4000 commands",[]{Engine engine(8);std::atomic<int> errors{0};std::vector<std::jthread> producers;for(int p=0;p<4;++p)producers.emplace_back([&,p]{try{std::vector<std::future<Response>> results;for(int i=0;i<1000;++i)results.push_back(engine.submit(buy(static_cast<Id>(p*1000+i+1),100,1)));for(auto& result:results)if(!result.get().ok)++errors;}catch(...){++errors;}});producers.clear();CHECK(errors==0);auto d=engine.submit(Depth{}).get();CHECK(d.bids[0].quantity==4000);CHECK(d.bids[0].orders==4000);});
 test("engine destructor drains pending futures",[]{std::future<Response> f;{Engine engine;f=engine.submit(buy(1,100,4));}CHECK(f.get().ok);});
 test("engine rejects submission after close",[]{Engine engine;engine.close();bool threw=false;try{engine.submit(Depth{});}catch(std::runtime_error const&){threw=true;}CHECK(threw);});
 std::cout << count << " tests passed\n";
 } catch(std::exception const& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
