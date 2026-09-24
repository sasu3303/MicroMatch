#pragma once
#include "book.hpp"
#include "queue.hpp"
#include <future>
#include <memory>
#include <thread>

namespace micromatch {
class Engine {
    struct Job { Command command; std::promise<Response> result; };
    OrderBook book_;
    BoundedQueue<std::unique_ptr<Job>> queue_;
    std::jthread worker_;
public:
    explicit Engine(std::size_t capacity = 1024) : queue_(capacity), worker_([this] {
        std::exception_ptr failure;
        while (auto job = queue_.pop()) {
            if (failure) { (*job)->result.set_exception(failure); continue; }
            try { (*job)->result.set_value(book_.execute((*job)->command)); }
            catch (...) {
                // Internal failures (e.g. allocation failure) may leave partial state.
                // Stop acceptance and fail remaining work instead of reusing that state.
                failure = std::current_exception();
                (*job)->result.set_exception(failure);
                queue_.close();
            }
        }
    }) {}
    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;
    ~Engine() { close(); } // jthread joins before queue_ and book_ are destroyed.
    void close() { queue_.close(); }
    std::future<Response> submit(Command command) {
        auto job = std::make_unique<Job>();
        job->command = std::move(command);
        auto future = job->result.get_future();
        if (!queue_.push(std::move(job))) throw std::runtime_error("Engine closed");
        return future;
    }
};
}
