# Verified results

Executed in the authoring Linux environment using c++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0 and CMake Release builds for benchmarks. CPU reported: AMD EPYC 9V74 80-Core Processor.

## Functional verification

| Check | Result |
|---|---|
| Named C++ cases | 24 passed |
| Independent matching reference | 10,000 seeded commands; exact response and depth comparison after each operation |
| Concurrent submission | Four producers, 4,000 total commands, queue capacity 8; all accepted and aggregate quantity correct |
| TCP integration | Fragmentation, coalescing, matching, cancellation, malformed input, oversize frames, partial disconnect, reconnect passed |
| Release CTest | Both targets passed |
| AddressSanitizer + UndefinedBehaviorSanitizer | Both targets passed with `ASAN_OPTIONS=detect_leaks=0` |
| ThreadSanitizer | Both targets passed |
| Readable CLI demo | Ran successfully; output captured |

The first ASan/UBSan execution with default leak checking could not complete its LeakSanitizer phase because this environment uses tracing/ptrace. Leak detection was explicitly disabled for the reported passing run. **No passing leak-check result is claimed.** GDB configuration and GitHub Actions workflow are included but were not executed here. Windows and macOS were not tested.

## Benchmark measurements

Three trials were recorded for each workload. Values below are medians of the three trial results, not a pooled distribution.

| Metric | Measured result |
|---|---:|
| Core workload commands per trial | 200,000 |
| Core p95 command duration | 71 ns |
| Core throughput (separate pass) | 12,357,905 commands/s |
| Cancellation workload starting orders | 20,000 across 100 price levels |
| Vector scan + erase cancellation p95 | 5.428 microseconds |
| Indexed list cancellation p95 | 0.471 microseconds |

### Core workload

A repeatable four-command cycle adds an ask, matches it with a buy, adds a bid, then cancels the bid. There is **at most one active resting order**. Half the added orders cause a trade or later cancellation; the book ends empty. Accepted-ID history grows through the run. These tiny-book results are useful to inspect overhead, but are not representative of a deep or busy market.

One independent warmup book processes 1,000 commands. Per-operation latency includes core execution, its allocations, and clock overhead. Response destruction is outside the per-operation timer. Throughput uses a separate pass without per-command timers and includes response destruction. Parser, futures, queue, TCP, disk, and real market-data work are excluded.

### Cancellation workload

Populate 20,000 buy orders at 100 prices. Shuffle IDs using seed 2027, then cancel every order from an independent vector baseline and the actual indexed book in interleaved order. Each baseline cancellation linearly searches and shifts the vector; each indexed cancellation uses the hash locator, price tree, and list unlink. Setup is excluded; per-operation timing overhead is included. Both books finish empty and each cancellation is checked.

The book shrinks during the measurement. This is a synthetic algorithm comparison, not an optimized-before/after production deployment. There is no claim that the vector baseline implements all exchange functionality.

### Interpretation

Results are local, single-thread core measurements under uncontrolled host scheduling. No CPU pinning, cache isolation, network-load benchmark, or production validation was performed. Do not call these exchange latency, end-to-end order latency, or production throughput. Reproduce on your own machine and use your own measured results.

Raw trial values and environment information are in `evidence/benchmarks.json`; test logs and the readable demo are in `evidence/`.

## Project rename

The project is named MicroMatch. Historical test-log paths and the demo banner
have been normalized to the current name; recorded benchmark measurements are unchanged.

Rename verification on 2026-09-24: the CMake Release build succeeded with GCC 13.3.0.
All 24 C++ cases and both CTest targets (matching/concurrency and TCP protocol) passed.
The renamed demo and three-trial benchmark script also ran successfully.
Sanitizers were not rerun for this rename; the sanitizer results above are historical.
