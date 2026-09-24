# Architecture and tradeoffs

## Matching rules

One book represents one instrument. Bids are ordered high to low; asks low to high. Orders at a price are matched oldest first. A limit order crosses only at its limit or better. Market orders take available liquidity and expire the rest. The price of each fill is the resting order's price.

For example, sell order A at 10000 arrives before sell order B at 10000. A buy for more than A's quantity consumes A first, then B. A later sell at 9990 would execute before both because better price takes priority over arrival time.

This is a simplified matching model, not a description of any specific firm's exchange connectivity or every exchange's rules.

## Containers and complexity

| Structure | Purpose | Tradeoff |
|---|---|---|
| `std::map<Price, list<Order>>` | Ordered price levels | Tree lookup O(log P); pointer-heavy, not cache-optimal |
| `std::list<Order>` | FIFO per price | Stable iterators and O(1) unlink once located; per-node allocations |
| `unordered_map<Id, Locator>` | Find active order | Average O(1) lookup; worst case linear hashing behavior |
| `unordered_set<Id>` | Reject reused accepted IDs | Retained for session, so memory grows until max_seen |
| `deque<unique_ptr<Job>>` | Bounded work queue | Move-only ownership transfer, mutex/CV synchronization |

Cancellation is average O(1) ID lookup plus O(log P) price lookup and O(1) list unlink. It is not unconditionally O(1). Matching cost grows with orders/levels consumed and hash-index maintenance. Depth aggregation visits orders in requested levels rather than maintaining cached aggregate quantities.

The cancellation benchmark deliberately contrasts the real indexed book with an O(N) vector scan plus erase/shift baseline. It is an algorithm comparison under a synthetic shrinking-book workload; the baseline does not implement an alternative full exchange engine.

## Memory lifetime

Containers own orders. The active index stores non-owning iterators into those lists. Inserting another list node leaves existing iterators valid; erasing the referenced node invalidates that iterator. The code removes the index entry before erasing a fully filled order. OrderBook copying/moving is disabled to prevent locators accidentally referring to another object's nodes.

Jobs use `unique_ptr` because ownership moves from submitter to queue to worker. Results move through promise/future. Socket wrappers close their owned descriptor and cannot be copied. Stack variables and standard containers replace manual new/delete.

Normal validation rejections occur before book mutation. The implementation does not promise strong transaction rollback under allocation failure in the middle of a multi-fill command. Engine treats an internal exception as fatal: closes the queue, fails outstanding work, and rejects new submissions. Direct OrderBook users must likewise stop using the book after an internal exception. Validation errors are ordinary responses and do not poison the engine.

## Concurrency

One matching thread owns the book. Producers enqueue commands under a mutex. Consumers/producers wait with condition-variable predicates to handle spurious wakeups and avoid lost notifications. A full queue blocks producers instead of growing without bound. Closing rejects future pushes, wakes waiters, and drains queued work.

`Engine` closes the queue in its destructor. Its `jthread` member is destroyed (and joined) before the queue/book members. As with most C++ objects, callers must stop using an Engine before destroying it. `close()` can run concurrently with submission; concurrent destruction of an object still in use is not supported.

Acceptance order is queue insertion order. The order between simultaneous producers is scheduler-dependent; once accepted, matching/replay is deterministic. A single owner is chosen for reasoning simplicity, not as a claim of maximum exchange throughput. The standalone queue supports multiple producers; the networking demo serves one connection at a time.

## TCP

TCP is a byte stream, so one recv call may contain part of a command or several commands. The gateway accumulates bytes until newline, validates the frame limit, parses strict integers, and emits one JSON line per command. It loops over partial sends and handles interrupted syscalls. Connection state is owned by a RAII socket. Client disconnect discards incomplete input; complete earlier commands remain applied.

The server binds only loopback and is deliberately not a public, authenticated service. A 60-second socket operation timeout bounds most idle waits, but it is not a complete defense against slow-drip peers. No TLS, UDP market-data feed, reconnect deduplication, durable journal, or production protocol is implemented.

## Constraints and unimplemented areas

No persistence: restart discards the book and seen IDs. No replace/amend behavior, risk checks, fees, market sessions, tick-size tables, real market data, multiple instruments, account ownership, or self-trade prevention. A maximum active-order condition rejects even incoming orders that might otherwise immediately execute. Fixed numeric bounds keep aggregate quantities within int64 under default capacity.

The project illustrates memory ownership but does not implement custom allocators or lock-free structures. Profiling and cache optimization are learning exercises, not verified improvements already achieved.

## Reference material

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines): RAII, resource ownership, concurrency.
- [CMake build command](https://cmake.org/cmake/help/latest/manual/cmake.1.html): configuration and builds.
- [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html): test execution.
