#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace micromatch {
// Bounded multi-producer queue. Close rejects new pushes and drains existing work.
// The owner must close and join all users before destroying the queue.
template<class T> class BoundedQueue {
    std::mutex mutex_;
    std::condition_variable readable_, writable_;
    std::deque<T> data_;
    std::size_t capacity_;
    bool closed_ = false;
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (!capacity) throw std::invalid_argument("Queue capacity must be positive");
    }
    bool push(T item) {
        std::unique_lock lock(mutex_);
        writable_.wait(lock, [&] { return closed_ || data_.size() < capacity_; });
        if (closed_) return false;
        data_.push_back(std::move(item));
        lock.unlock(); readable_.notify_one();
        return true;
    }
    std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        readable_.wait(lock, [&] { return closed_ || !data_.empty(); });
        if (data_.empty()) return std::nullopt;
        auto item = std::move(data_.front()); data_.pop_front();
        lock.unlock(); writable_.notify_one();
        return item;
    }
    void close() {
        { std::lock_guard lock(mutex_); closed_ = true; }
        readable_.notify_all(); writable_.notify_all();
    }
};
}
