# xtd

A header-only C++23 concurrency library in the `xtd` namespace.

`xtd` currently provides two complementary primitives:

| Primitive | Purpose | Documentation |
|---|---|---|
| `xtd::channel<T>` | Thread-safe, message-oriented producer/consumer communication | [Channel API reference](src/channel/channel.md) |
| `xtd::pipeline` | Byte-oriented streaming, incremental parsing, and backpressure | [Pipeline API reference](src/pipeline/pipeline.md) |

## Features

### Channel

* Bounded and unbounded channels.
* Multi-producer, multi-consumer access.
* FIFO ordering by successful enqueue order.
* Blocking `push`, `emplace`, and `read` operations.
* Non-blocking `try_push`, `try_emplace`, and `try_read` operations.
* Cooperative cancellation through `std::stop_token`.
* Graceful writer completion while preserving buffered values.
* Backpressure for bounded channels.
* In-place construction with `emplace`.
* Copyable and move-only value support.
* Synchronized backlog inspection through `reader.size()`.

See the complete [Channel API reference](src/channel/channel.md).

### Pipeline

* Single logical writer and single logical reader with concurrent execution.
* Configurable fixed-size segments.
* Pause and resume thresholds for writer backpressure.
* Incremental writes, including writes larger than the pause threshold.
* Cancellation-aware reads and writes through `std::stop_token`.
* `read()` and `read_at_least()` operations.
* Explicit consumed and examined positions through `advance()`.
* Non-owning segmented byte views without flattening the complete buffer.
* Slicing, searching, copying, and string conversion across segment boundaries.
* Internal fixed-buffer pooling for steady-state reuse.
* Completion semantics for both reader and writer endpoints.
* File and socket ingestion helpers through `xtd::pipe_utils`.

See the complete [Pipeline API reference](src/pipeline/pipeline.md).

# Contributing

If you enjoy modern C++, concurrency, or simply like building high-quality libraries, you're welcome here.

This project is still evolving, and there are plenty of opportunities to make it better—whether that's improving the implementation, optimizing performance, expanding the documentation, fixing bugs, or creating practical examples.

Contributions of every size are appreciated.

Even if you're not sure your contribution is valuable, open an issue or start a discussion. Fresh perspectives often lead to the best improvements.

If this project interests you, feel free to fork it, experiment, and submit a pull request. We'd love to see what you build with **xtd**.

---

## Requirements

* A C++23 compiler.
* POSIX threads when building the threaded samples, tests, and benchmarks.
* The repository scripts currently use GCC 15 through `g++-15`.

The library is header-only. Add `src` to the compiler include path:

```bash
g++-15 -std=c++23 -pthread -I./src app.cpp -o app
```

## Samples

| Sample | Demonstrates | Run |
|---|---|---|
| [channel.cpp](samples/channel.cpp) | Bounded or unbounded channels, multiple writers and readers, `emplace`, completion, and backlog validation | `./samples/run.sh channel.cpp bounded` |
| [pipeline.cpp](samples/pipeline.cpp) | Concurrent byte production, newline-delimited parsing, segmented reads, and backpressure | `./samples/run.sh pipeline.cpp` |
| [stdin.cpp](samples/stdin.cpp) | Inter-thread console communication using a pipeline, with the main thread consuming data | `./samples/run.sh stdin.cpp` |
| [Simple Chat Server](samples/chat.md) | A terminal-based TCP chat server with multiple clients and `/name` support | `./samples/run.sh chat.cpp` |

Build every top-level sample with:

```bash
./samples/build_all.sh
```

## Benchmarks

The benchmarks use [nanobench](https://github.com/martinus/nanobench) and record machine information together with throughput results.

### Channel Benchmark

The channel benchmark covers:

* Single-thread bounded channel.
* Single-thread unbounded channel.
* Multi-thread bounded channel with background consumers.
* Multi-thread unbounded channel with background consumers.

Source: [benchmarks/channels.cpp](benchmarks/channels.cpp)  
Current recorded output: [benchmarks/results/channels.md](benchmarks/results/channels.md)

### Pipeline Benchmark

The pipeline benchmark transfers data between a writer and a background reader using these write sizes:

* 1 KiB
* 2 KiB
* 4 KiB
* 8 KiB
* 16 KiB
* 32 KiB

Source: [benchmarks/pipelines.cpp](benchmarks/pipelines.cpp)  
Current recorded output: [benchmarks/results/pipelines.md](benchmarks/results/pipelines.md)

Run all benchmarks with:

```bash
./benchmarks/all.sh
```

Benchmark results are machine- and configuration-dependent. Review the machine specification and stability warnings included in each output file before comparing results.

## Tests and CI

Channel and pipeline tests are available under [`tests`](tests).

Run all tests with:

```bash
./tests/all.sh
```

GitHub Actions currently builds and runs the tests and benchmarks with GCC 15:

* [Test workflow](.github/workflows/run_tests.yml)

## Contributing

If you enjoy modern C++, concurrency, or building high-quality libraries, contributions are welcome.

The project is still evolving. Useful contributions include implementation improvements, performance work, documentation, tests, portability fixes, practical examples, and new ideas that fit the existing APIs.

Contributions of every size are appreciated. Open an issue or discussion when an idea needs design feedback, or submit a pull request when it is ready for review.

## Project TODO

### Documentation

Suggested next step: add a short ownership, lifetime, and synchronization diagram for each primitive.

* [x] Complete API documentation for all public types.
* [ ] Add an architecture overview.
* [ ] Explain important design decisions and trade-offs.
* [ ] Document common concurrency patterns.
* [ ] Document endpoint ownership and lifetime rules in the README.
* [ ] Add a migration and versioning policy.
* [ ] Expand benchmark methodology and reproducibility notes.
* [ ] More...

### Samples

Suggested next step: add one small real-world example for each primitive before adding larger demos.

* [x] Bounded and unbounded channel producer/consumer.
* [x] Multi-producer, multi-consumer channel.
* [x] Pipeline producer/consumer.
* [x] STDIN pipeline.
* [x] TCP chat server.
* [ ] Add a cancellable worker-pool channel example.
* [ ] Add a length-prefixed binary protocol parser.
* [ ] Add a pipeline file-processing example.
* [ ] Add a TCP echo or request/response pipeline example.
* [ ] More...

### Features

Suggested next step: improve operation results before adding larger abstractions, so cancellation, completion, full capacity, and errors can be distinguished cleanly.

* [x] Cooperative channel cancellation with `std::stop_token`.
* [x] Cooperative pipeline cancellation with `std::stop_token`.
* [x] Non-blocking channel operations.
* [x] Bounded-channel and pipeline backpressure.
* [x] In-place channel construction.
* [x] Segmented pipeline reads and parsing utilities.
* [ ] Add `std::stop_token` support to `pipe_utils`.
* [ ] Add timed channel send and receive operations.
* [ ] Add coroutine-friendly adapters.
* [ ] Add richer operation status types.
* [ ] Add additional pipeline parsing helpers or operators.
* [ ] Add an optional executor abstraction.
* [ ] Add configurable scheduling or wake-up strategies.
* [ ] Add more compile-time validation.
* [ ] More...

### Performance

Suggested next step: separate latency, throughput, contention, and allocation benchmarks so each optimization has a clear target.

* [x] Channel throughput benchmarks.
* [x] Bounded and unbounded channel comparisons.
* [x] Single-thread and multi-thread channel scenarios.
* [x] Pipeline throughput benchmarks across multiple write sizes.
* [x] Machine specification and stability warnings in benchmark output.
* [ ] Add producer and consumer scaling benchmarks.
* [ ] Add channel latency distribution benchmarks.
* [ ] Add pipeline frame-parsing benchmarks.
* [ ] Add allocation and peak-memory measurements.
* [ ] Publish cross-platform benchmark results.
* [ ] Compare against similar C++ libraries.
* [ ] Add long-running stress tests with millions of messages.
* [ ] Add automated regression thresholds.
* [ ] More...

### Quality

Suggested next step: run the existing tests under sanitizers before expanding the compiler matrix.

* [x] Channel unit tests.
* [x] Pipeline unit tests.
* [x] GCC 15 CI for tests and benchmarks.
* [ ] Increase unit-test coverage for edge cases and failure paths.
* [ ] Add deterministic concurrency stress tests.
* [ ] Add fuzz testing for segmented parsing and position validation.
* [ ] Add Thread Sanitizer CI.
* [ ] Add Address Sanitizer CI.
* [ ] Add Undefined Behavior Sanitizer CI.
* [ ] Add static analysis.
* [ ] Add coverage reports.
* [ ] Add continuous benchmarking.
* [ ] More...

### Tooling

Suggested next step: add a minimal CMake interface target while keeping direct header inclusion supported.

* [x] Shared compiler flags file.
* [x] Scripts for samples, tests, and benchmarks.
* [x] GitHub Actions workflow.
* [ ] Add CMake package installation.
* [ ] Add a CMake interface target.
* [ ] Add a Conan package.
* [ ] Add a vcpkg port.
* [ ] Expand the CI compiler and operating-system matrix.
* [ ] Publish a compiler support matrix.
* [ ] Add release automation.
* [ ] More...

### Nice to Have

Suggested next step: prioritize contributor guidance and benchmark visualization before branding work.

* [ ] Add a contribution guide with local validation commands.
* [ ] Add interactive examples.
* [ ] Add more real-world demos.
* [ ] Add a benchmark dashboard.
* [ ] Add performance visualizations.
* [ ] Add a project website.
* [ ] Add a logo and basic branding.
* [ ] More...

## License

`xtd` is available under the [MIT License](LICENSE).