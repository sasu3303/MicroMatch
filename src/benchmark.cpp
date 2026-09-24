#include "micromatch/book.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace micromatch;
using Clock=std::chrono::steady_clock;
int main(int argc,char** argv) {
 try {
    std::size_t n=200000;
    if(argc>2)throw std::invalid_argument("Usage: micromatch_bench [operations]");
    if(argc==2){std::string s=argv[1];std::size_t used=0;auto v=std::stoull(s,&used);if(used!=s.size()||v<1000||v>1000000)throw std::invalid_argument("Operations must be 1000..1000000");n=static_cast<std::size_t>(v);}
    n-=n%4;
    std::vector<Command> commands;commands.reserve(n);Id id=1;
    for(std::size_t i=0;i<n/4;++i){Price p=10000+static_cast<Price>(i%100);commands.emplace_back(Add{id++,Side::Sell,p,10});commands.emplace_back(Add{id++,Side::Buy,p,10});Id cancel_id=id++;commands.emplace_back(Add{cancel_id,Side::Buy,p-1,20});commands.emplace_back(Cancel{cancel_id});}
    // Warmup uses an independent book and does not pollute measured state.
    {OrderBook warm;for(std::size_t i=0;i<1000;++i)warm.execute(commands[i]);}
    std::vector<double> samples;samples.reserve(n);std::uint64_t fills=0;
    OrderBook measured;
    for(auto const& command:commands){auto start=Clock::now();auto r=measured.execute(command);auto end=Clock::now();if(!r.ok)throw std::runtime_error("Rejected benchmark command");fills+=r.trades.size();samples.push_back(std::chrono::duration<double,std::nano>(end-start).count());}
    OrderBook throughput;std::uint64_t checksum=0;
    auto start=Clock::now();for(auto const& command:commands){auto r=throughput.execute(command);if(!r.ok)throw std::runtime_error("Rejected benchmark command");checksum+=r.trades.size();}auto end=Clock::now();
    if(!measured.invariant()||!throughput.invariant()||fills!=n/4||checksum!=fills||measured.active_count()!=0)throw std::runtime_error("Benchmark correctness check failed");
    double elapsed=std::chrono::duration<double>(end-start).count();
    std::sort(samples.begin(),samples.end());
    auto percentile=[&](double p){return samples[static_cast<std::size_t>(std::ceil(p*static_cast<double>(n)))-1];};
    std::cout<<std::fixed<<std::setprecision(3)<<"{\n  \"operations\": "<<n<<",\n  \"trades\": "<<fills<<",\n  \"p50_ns\": "<<percentile(.50)<<",\n  \"p95_ns\": "<<percentile(.95)<<",\n  \"p99_ns\": "<<percentile(.99)<<",\n  \"throughput_ops_per_second\": "<<static_cast<double>(n)/elapsed<<",\n  \"throughput_elapsed_seconds\": "<<elapsed<<",\n  \"final_active_orders\": 0,\n  \"workload\": \"25% resting asks, 25% matching buys, 25% resting bids, 25% cancels; at most one active order\",\n  \"scope\": \"Single-thread in-process core; allocation included; no parsing, queue, TCP, disk or realistic deep book\"\n}\n";
 }catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
