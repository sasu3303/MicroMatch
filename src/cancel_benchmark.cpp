#include "micromatch/book.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
using namespace micromatch;
using Clock=std::chrono::steady_clock;
int main() {
    try {
        constexpr std::size_t n=20000;
        std::vector<Id> ids(n);std::iota(ids.begin(),ids.end(),Id{1});std::mt19937 rng(2027);std::shuffle(ids.begin(),ids.end(),rng);
        std::vector<Order> baseline;baseline.reserve(n);OrderBook indexed;
        for(Id id=1;id<=n;++id){Order o{id,Side::Buy,10000+static_cast<Price>(id%100),1};baseline.push_back(o);if(!indexed.add({o.id,o.side,o.price,o.quantity}).ok)throw std::runtime_error("Setup rejected");}
        std::vector<double> slow,fast;slow.reserve(n);fast.reserve(n);
        for(Id id:ids){
            auto start=Clock::now();auto it=std::find_if(baseline.begin(),baseline.end(),[id](auto const& o){return o.id==id;});if(it==baseline.end())throw std::runtime_error("Baseline missing ID");baseline.erase(it);auto end=Clock::now();slow.push_back(std::chrono::duration<double,std::nano>(end-start).count());
            start=Clock::now();auto r=indexed.cancel(id);end=Clock::now();if(!r.ok||r.remaining!=1)throw std::runtime_error("Indexed cancel incorrect");fast.push_back(std::chrono::duration<double,std::nano>(end-start).count());
        }
        if(!baseline.empty()||indexed.active_count()!=0||!indexed.invariant())throw std::runtime_error("Final book mismatch");
        std::sort(slow.begin(),slow.end());std::sort(fast.begin(),fast.end());auto p95=static_cast<std::size_t>(std::ceil(.95*n))-1;
        std::cout<<std::fixed<<std::setprecision(3)<<"{\n  \"resting_orders\": "<<n<<",\n  \"price_levels\": 100,\n  \"cancellations\": "<<n<<",\n  \"seed\": 2027,\n  \"vector_scan_erase_p95_ns\": "<<slow[p95]<<",\n  \"indexed_list_erase_p95_ns\": "<<fast[p95]<<",\n  \"final_active_orders_both\": 0,\n  \"scope\": \"Shrinking single-thread book; interleaved vector-scan/erase and indexed-list cancellation; clock overhead included; no queue or network\"\n}\n";
    }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
