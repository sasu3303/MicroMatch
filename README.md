# MicroMatch - C++ Exchange Simulator

A C++20 learning project built around one instrument's order book. Submit limit/market orders, observe price-time-priority matching, cancel orders, inspect depth, and replay a deterministic scenario. A bounded queue connects producers to one matching thread. A local TCP gateway demonstrates message framing and socket ownership.

This is an exchange **simulation**, not a trading strategy, broker integration, or production exchange. It uses no accounts, live market feeds, money, or external trading services.

## What you learn

| Gap | Implemented example |
|---|---|
| Modern C++ | Value types, const references, templates, variants, move-only jobs, unique_ptr, futures, jthread |
| Data structures | Ordered maps for prices, FIFO lists for orders, hash lookup for cancellation |
| Memory ownership | Container-owned orders, stable list iterators, noncopyable book, RAII sockets |
| Concurrency | Bounded producer/consumer queue, mutexes, predicate waits, backpressure, shutdown |
| Systems/networking | Loopback TCP, fragmented/coalesced messages, partial sends, disconnects, input limits |
| Performance | Release benchmarks, percentile measurements, vector-scan vs indexed cancellation |
| Testing/debugging | Independent reference model, seeded randomized checks, sanitizers, CMake/CTest, VS Code GDB setup |

The learning guide includes independent exercises, and the verification notes document the scope of the recorded tests and benchmarks.

## Windows + VS Code: start here

Use **Ubuntu through WSL** for the complete project, including its POSIX TCP server. Open `docs/WINDOWS_VSCODE.md` for exact steps. You do not need Python 3.12 specifically: the small helper scripts use Python 3.9+, while the core application needs a C++20 compiler.

## Linux/WSL quick start

Install a C++20-capable compiler (tested with GCC 13), CMake 3.20+, Python 3, and optionally GDB. From the extracted `MicroMatch` folder:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 scripts/demo.py
```

The demo reads `data/demo.txt`, runs the real C++ engine, and explains its JSON responses. See `evidence/demo-output.txt` for a captured run before installing anything.

## First example

```bash
./build/micromatch
```

Type these commands, pressing Enter after each:

```text
LIMIT 1 SELL 10000 10
LIMIT 2 SELL 10000 20
LIMIT 3 BUY 10000 15
BOOK
CANCEL 2
BOOK
QUIT
```

The buy fills seller 1's 10 units first, then 5 units from seller 2. The first BOOK has 15 units remaining at 10000 ticks. Cancelling seller 2 empties the book. Prices are integer ticks; the application does not assume a currency or tick-to-dollar conversion.

## Protocol

| Command | Meaning |
|---|---|
| `LIMIT id BUY price quantity` | Buy up to the limit, rest any unmatched remainder |
| `LIMIT id SELL price quantity` | Sell down to the limit, rest any unmatched remainder |
| `MARKET id BUY quantity` | Buy available asks; expire any unmatched remainder |
| `MARKET id SELL quantity` | Sell into available bids; expire any unmatched remainder |
| `CANCEL id` | Cancel the remaining quantity of an active order |
| `BOOK` or `BOOK n` | Best n levels per side (default 10; maximum 100) |
| `QUIT` | End the CLI or close the TCP client connection |

One newline-terminated JSON response per command. Valid sides are uppercase BUY/SELL. Positive order IDs are unique for the entire in-memory session, including filled/cancelled IDs. The incoming order trades at the resting maker's price. Time priority means engine acceptance order, not a client-supplied timestamp.

Quantity: 1–1,000,000. Limit price: 1–1,000,000,000 ticks. Default book limits: 100,000 active orders, 2,000,000 accepted IDs. At active capacity, all new orders are rejected, even potentially immediately executable ones. Book state is in memory and disappears on process exit. No amendment, multi-symbol routing, self-trade prevention, auctions, risk engine, or persistence is implemented.

## TCP mode (Linux/WSL/macOS source path)

Terminal 1:

```bash
./build/micromatch_server 9000
```

Terminal 2:

```bash
python3 scripts/client.py
```

This server binds **127.0.0.1**, serves one client at a time, and closes an idle/blocked socket after a 60-second operation timeout. Commands may be fragmented or coalesced across TCP reads. Frames over 256 bytes are rejected and the connection closes. An incomplete final frame is discarded on disconnect. `QUIT` closes that client; the server accepts the next client. Stop the server with Ctrl+C. `--once` after the port exits after one connection; port 0 selects an ephemeral port for tests.

This is a socket protocol, not HTTP. Opening port 9000 in a browser is not the way to use it. It is local-only and has no authentication or TLS.

## Tests, benchmarks, and diagnostics

```bash
./build/micromatch_tests
python3 scripts/run_benchmarks.py
```

The benchmark script records three runs of each workload plus environment metadata. The core benchmark uses a tiny book; the cancellation benchmark starts with 20,000 resting orders and compares a vector scan/erase baseline with the actual indexed order book. Neither measures TCP latency or real trading performance.

Debug build and VS Code F5 configuration:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j 4
```

Memory/undefined behavior checks:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build-asan -j 4
ctest --test-dir build-asan --output-on-failure
```

Race checks in a separate build:

```bash
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON
cmake --build build-tsan -j 4
ctest --test-dir build-tsan --output-on-failure
```

Do not combine ASan and TSan. The authoring container cannot run LeakSanitizer under its tracing environment; the recorded ASan run disables leak detection only. See `docs/VERIFICATION.md` for what actually passed. Keep the default leak checks enabled on your own compatible machine.

## Read next

1. `docs/LEARNING_GUIDE.md`: seven hands-on sessions and ownership exercises.
2. `docs/ARCHITECTURE.md`: data structures, complexity, lifetime, threading, limitations.
3. `docs/VERIFICATION.md`: measured evidence and scope.
4. `docs/RESUME_NOTES.md`: claims you can develop and substantiate.

Only standard C++ and OS socket APIs are used. No external C++ dependency downloads are required. Native Windows core compilation is intended through CMake/MSVC, but was not tested; TCP and the supplied GDB launch configuration target Linux/WSL. GitHub Actions is configured but has not been run in your account.
