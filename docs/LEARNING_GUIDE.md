# Learn MicroMatch by changing it

This project is intended to help you move from Java/Python/backend work toward C++ systems programming. Read a small piece, run it, predict its behavior, then change it. Use the exercises to practice implementation and debugging.

## Session 1 - Understand the market model

Run `python3 scripts/demo.py`. Read each LIMIT command before its result. A bid is an offer to buy; an ask is an offer to sell. A marketable incoming order is the taker, and a resting order it consumes is the maker. This simulator uses the maker's price.

Predict this before running it:

```text
LIMIT 1 SELL 10005 3
LIMIT 2 SELL 10000 4
LIMIT 3 BUY 10010 5
BOOK
```

Expected: fill 4 at 10000 from maker 2, then 1 at 10005 from maker 1. Two units remain at ask 10005. This demonstrates price priority, not FIFO across different prices.

Your task: add a scenario to `data/` for a sell sweeping two bid levels, and explain the expected fills yourself.

## Session 2 - Translate your Java/Python mental model

Read `book.hpp`.

- `Order` is a value, not a garbage-collected object reference. Copying it copies its fields.
- `Add const&` borrows a read-only reference for a call. It does not transfer ownership.
- `int64_t` has a fixed range; Python integers do not have the same overflow model.
- `std::variant` represents one of a fixed set of command types.
- `std::unique_ptr<Job>` has one owner. `std::move` permits ownership transfer; it is not inherently a copy or an instruction to physically move every byte.
- A destructor releases resources when lifetime ends. RAII is the organizing rule, rather than waiting for a garbage collector.

Your task: explain why copying OrderBook is explicitly disabled. Find which field would become dangerous if a naive copy were allowed.

## Session 3 - Follow a match in the debugger

Open `src/book.cpp` and use the supplied VS Code GDB launch configuration. Break inside `OrderBook::match`. Follow one partial fill and one full fill. Observe the map iterator, list front, index entry, and remaining quantity.

Your task: draw the state on paper after each command, then compare with BOOK. Explain when `resting` becomes invalid and why the code does not use it after `pop_front()`.

Add your own regression test for a cancellation immediately after a partial fill. The existing tests cover related cases; write and explain yours without copying the implementation.

## Session 4 - Learn the queue

Read `queue.hpp`, then `engine.hpp`. Compare `std::lock_guard`/`unique_lock` with Java synchronization. Notice that the condition variable always has a predicate and waits release/reacquire the mutex.

Explain:

1. What stops the queue from growing indefinitely?
2. Why must close wake both producers and consumers?
3. Why is the book itself not protected by a mutex?
4. What does a producer receive if shutdown occurs while it is waiting to submit?
5. Why must the worker join before the queue is destroyed?

Your task: add a thread-safe high-water-mark metric to BoundedQueue. Write a test that fills a known-capacity queue and checks the metric. Do not introduce unsynchronized reads of queue internals.

## Session 5 - Understand TCP framing

Run the local server and Python client. Then read `tests/test_tcp.py` and `src/server.cpp`. Observe why one socket read does not equal one command and why a response may require multiple sends.

Your task: add a protocol `PING` command with a fixed response and tests for a fragmented PING frame. Keep invalid commands rejected and do not reflect arbitrary input inside unescaped JSON.

Explain why this single-client gateway is not a scalable trading gateway. Propose a design for multiple clients, but do not add one until you can state its ownership and shutdown rules.

## Session 6 - Measure rather than guess

Run the Release benchmarks. Read `evidence/benchmarks.json` and `docs/VERIFICATION.md`. Compare the tiny-book core workload with the deeper cancellation workload. Neither is a production workload.

Your task: add a benchmark with 10,000 resting orders and market orders sweeping several price levels. Record workload, compiler, CPU, p50/p95/p99, and matching correctness. Avoid benchmarking a Debug or sanitizer build as if it were Release.

Explain why list nodes help iterator stability but may hurt cache locality. Only after measuring, consider cached level totals or a pool allocator. Do not claim a cache optimization before implementing and measuring it.

## Session 7 - Make one meaningful feature yours

Choose **one**:

- Add order replacement, explicitly specifying when an order loses FIFO priority.
- Add per-price aggregate quantity caching and test every fill/cancel path.
- Add a second instrument with independent books and explicit routing.
- Add a persisted command journal and deterministic replay, with a clearly defined durability boundary.

Before coding, write acceptance criteria and at least two edge cases. Implement it, run the tests/sanitizers, and record a bug you diagnosed. Keep notes on the implementation decisions and debugging results.

## Three-minute interview demonstration

1. Show a partial fill that respects price-time priority.
2. Show a cancellation and explain its data structures and complexity.
3. Explain single-owner matching and bounded-queue backpressure.
4. Show one test you wrote and one benchmark you reproduced.
5. State one limitation and how you would improve it.

You should be able to do this without reading a script before adding strong independent implementation claims to your resume.
