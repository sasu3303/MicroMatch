# Resume use and evidence

Project name: **MicroMatch - C++ Exchange Simulator**.

The code demonstrates C++20, price-time matching, data structures, RAII, producer/consumer synchronization, local TCP framing, CMake, testing, and benchmarking.

## Current facts

- Limit and market orders, cancellation, partial fills, FIFO price-level ordering, and depth snapshots are implemented.
- 24 named C++ cases passed, including a 10,000-command independent-reference comparison and a four-producer, 4,000-command test.
- Local TCP protocol integration passed.
- Address/undefined-behavior sanitizer checks passed with leak detection disabled because the environment's tracing prevented LeakSanitizer from running.
- ThreadSanitizer checks passed for the tested suite. This is not a proof that all possible races are absent.
- Measured local benchmark results and limitations are in VERIFICATION.md and the evidence folder.
- No live trading, broker connection, real market data, cloud deployment, custom allocator, or lock-free implementation exists.

## How to earn stronger resume bullets

After you understand the implementation, reproduce the results, and make an independent feature contribution, describe your actual work. Possible structure:

- Extended a C++20 exchange simulator with [your feature], preserving price-time priority and validating [your edge cases].
- Tested a bounded producer/consumer queue and single-owner matching engine using [checks you personally ran and understood].
- Compared indexed order cancellation with a vector-scan baseline on [your dataset], measuring [your results] under [your environment].

Reproduce benchmarks on your own machine and record the environment alongside each result. Avoid claims such as production HFT system, nanosecond network latency, lock-free engine, guaranteed race-free execution, or profitable trading algorithm.

For the Old Mission application, this is more directly relevant to the described C++ work than adding another CRUD dashboard. The value comes from your ability to explain and extend it, not from the project name or an arbitrary resume score.
