# xtd Channels Reference

This page documents the public channel API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Headers and Types](#2-headers-and-types)
3. [`xtd::bounded_channel<T, capacity>`](#3-xtdbounded_channelt-capacity)
4. [`xtd::unbounded_channel<T>`](#4-xtdunbounded_channelt)
5. [`xtd::channel_writer<T>`](#5-xtdchannel_writert)
6. [`xtd::channel_reader<T>`](#6-xtdchannel_readert)
7. [Semantics and Guarantees](#7-semantics-and-guarantees)
8. [Complexity](#8-complexity)
9. [How-to Guides](#9-how-to-guides)
10. [Examples](#10-examples)

## 1) Overview

`xtd` channels are thread-safe message queues for producer/consumer workloads.

- `xtd::bounded_channel<T, capacity>` uses fixed-capacity storage and can block writers when full.
- `xtd::unbounded_channel<T>` uses dynamic storage and does not block writers due to capacity.
- Writers and readers are exposed via `writer()` and `reader()` endpoint objects.
- Completion is cooperative: after `complete()`, no more writes are accepted.

Primary use cases:

- Multi-producer, multi-consumer task dispatch.
- Backpressure (bounded channel).
- Graceful shutdown of read loops.

## 2) Headers and Types

### Namespace

```cpp
namespace xtd { /* ... */ }
```

### Main headers

```cpp
#include "channel/bounded_channel.h"
#include "channel/unbounded_channel.h"
```

### Public types

- `xtd::bounded_channel<T, capacity>`
- `xtd::unbounded_channel<T>`
- `xtd::channel_writer<T>`
- `xtd::channel_reader<T>`

## 3) `xtd::bounded_channel<T, capacity>`

### Synopsis

```cpp
namespace xtd {

template<class T, std::size_t capacity>
class bounded_channel {
public:
    bounded_channel();

    channel_writer<T>& writer() noexcept;
    channel_reader<T>& reader() noexcept;
};

}
```

### Template parameters

- `T`: message value type.
- `capacity`: number of buffered elements. Must be greater than 0.

### Behavior

- Storage is fixed-size and embedded in the channel.
- The bounded buffer is represented as `std::optional<T> m_buffer[capacity]`.
- `push()` / `emplace()` block when full, until space becomes available or channel completes.
- `try_push()` / `try_emplace()` never block and return `false` when full/completed.

### Notes

- Suitable when you need bounded memory usage and backpressure.
- Capacity is compile-time constant.
- Large capacities increase object size because storage is embedded in the channel object.

## 4) `xtd::unbounded_channel<T>`

### Synopsis

```cpp
namespace xtd {

template<class T>
class unbounded_channel {
public:
    unbounded_channel();

    channel_writer<T>& writer() noexcept;
    channel_reader<T>& reader() noexcept;
};

}
```

### Behavior

- Uses dynamic queue storage.
- Writer operations do not block due to capacity.
- Reader `read()` blocks while empty until data arrives or channel is completed.

### Notes

- Suitable when producer throughput can spike and buffering can grow.
- Requires attention to memory growth in long-running systems.

## 5) `xtd::channel_writer<T>`

The abstract write endpoint interface returned by `channel.writer()`.

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel_writer {
public:
    virtual ~channel_writer() = default;

    virtual bool push(const T& value) = 0;
    virtual bool push(T&& value) = 0;

    virtual bool try_push(const T& value) = 0;
    virtual bool try_push(T&& value) = 0;

    template<class... Args>
    bool emplace(Args&&... args);

    template<class... Args>
    bool try_emplace(Args&&... args);

    virtual void complete() = 0;
};

}
```

### Return values

- `push` / `emplace`: `true` if enqueued, `false` if channel is completed.
- `try_push` / `try_emplace`: `true` if enqueued, `false` if full (bounded only) or completed.

### Blocking behavior

- `push` / `emplace`
- `bounded_channel`: may block while full.
- `unbounded_channel`: does not block for capacity.

- `try_push` / `try_emplace`
- never block.

### Completion

- `complete()` marks channel complete for writing.
- Subsequent write attempts return `false`.
- `complete()` is safe to call multiple times.

## 6) `xtd::channel_reader<T>`

The abstract read endpoint interface returned by `channel.reader()`.

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel_reader {
public:
    virtual ~channel_reader() = default;

    virtual std::optional<T> try_read() = 0;
    virtual std::optional<T> read() = 0;
    virtual std::size_t size() const = 0;
};

}
```

### Return values

- `try_read()`: returns value if available, otherwise `std::nullopt`.
- `read()`: blocks while empty; returns `std::nullopt` when completed and empty.
- `size()`: current buffered item count.

### Blocking behavior

- `read()` blocks while empty and not completed.
- `try_read()` never blocks.

### Usage guidance

- Prefer `read()` for normal consumer loops.
- Use `try_read()` only for opportunistic polling, such as integrating with an external event loop where blocking is not acceptable.

## 7) Semantics and Guarantees

### Thread-safety

- Channels are designed for concurrent multi-producer and multi-consumer usage.
- Calling writer and reader operations from multiple threads is supported.

### Ordering

- FIFO order is preserved for the enqueue/dequeue sequence.

### Completion semantics

- Completion stops future writes.
- Buffered items remain readable after completion.
- Once buffer is drained, `read()` returns `std::nullopt`.

### Blocking matrix

| Operation | bounded_channel | unbounded_channel |
|---|---|---|
| `push`, `emplace` | Blocks when full | Does not block for capacity |
| `try_push`, `try_emplace` | Never blocks | Never blocks |
| `read` | Blocks when empty | Blocks when empty |
| `try_read` | Never blocks | Never blocks |

## 8) Complexity

Typical complexity (not counting blocking wait time):

- `push`, `try_push`, `emplace`, `try_emplace`: amortized O(1)
- `read`, `try_read`: O(1)
- `size`: O(1)

## 9) How-to Guides

### How to create a producer/consumer pair

```cpp
xtd::bounded_channel<int, 128> channel;
auto& writer = channel.writer();
auto& reader = channel.reader();
```

### How to stop consumers cleanly

```cpp
// producer side
writer.complete();

// consumer side
while (auto value = reader.read()) {
    // process *value
}
// loop exits when completed and drained
```

### How to avoid writer blocking in bounded mode

```cpp
if (!writer.try_push(item)) {
    // either full or completed
    // apply retry/backoff/drop policy
}
```

### How to transfer move-only or heavy objects

```cpp
struct Job {
    std::vector<int> payload;
};

xtd::unbounded_channel<Job> values;
auto value_writer = values.writer();
auto value_reader = values.reader();

Job a{{1, 2, 3}};
value_writer.push(a);                  // copy: lvalue overload (const T&)
value_writer.push(std::move(a));       // move: rvalue overload (T&&)
value_writer.emplace(Job{{4, 5, 6}});  // inline object creation, then move into channel
value_writer.emplace(std::vector<int>{7, 8, 9}); // in-place construction of Job

value_writer.complete();
while (auto job = value_reader.read()) {
    // consume *job
}

xtd::unbounded_channel<std::unique_ptr<Job>> pointers;
auto ptr_writer = pointers.writer();
auto ptr_reader = pointers.reader();

std::unique_ptr<Job> p(new Job{{10, 11, 12}});
ptr_writer.push(std::move(p));         // required move (unique_ptr is move-only)
ptr_writer.push(std::unique_ptr<Job>(new Job{{13, 14, 15}})); // inline creation + move

ptr_writer.complete();
while (auto job_ptr = ptr_reader.read()) {
    // job_ptr is std::optional<std::unique_ptr<Job>>
}
```

- `push(lvalue)` copies.
- `push(std::move(x))` moves.
- `emplace(args...)` constructs directly in the channel storage from `args...`.
- For move-only types (for example `std::unique_ptr<T>`), use `std::move` or an rvalue temporary.

### How to read move-only or heavy objects

```cpp
struct Job {
    std::vector<int> payload;
};

xtd::unbounded_channel<Job> values;
auto value_writer = values.writer();
auto value_reader = values.reader();

value_writer.emplace(std::vector<int>{1, 2, 3});
value_writer.complete();

while (auto item = value_reader.read()) {
    Job job = std::move(*item); // move out of std::optional<Job>
    // process job
}

xtd::unbounded_channel<std::unique_ptr<Job>> pointers;
auto ptr_writer = pointers.writer();
auto ptr_reader = pointers.reader();

ptr_writer.push(std::unique_ptr<Job>(new Job{{4, 5, 6}}));
ptr_writer.complete();

while (auto item = ptr_reader.read()) {
    std::unique_ptr<Job> job_ptr = std::move(*item); // required move
    // process *job_ptr
}
```

- `read()` returns `std::optional<T>` by value.
- For heavy or move-only `T`, move from `*item` (`std::move(*item)`) when taking ownership.
- If you only need to inspect fields, reading through `item->field` avoids an extra move/copy.

If you want to accumulate read items into a new vector, move each element from the optional:

```cpp
std::vector<Job> jobs;
while (auto item = value_reader.read()) {
    jobs.push_back(std::move(*item));
}

std::vector<std::unique_ptr<Job>> job_ptrs;
while (auto item = ptr_reader.read()) {
    job_ptrs.push_back(std::move(*item));
}
```

- Use `push_back(std::move(*item))` (or `emplace_back(std::move(*item))`) to avoid copies.
- For move-only types, moving is required.

### How to monitor backlog

```cpp
std::size_t queued = reader.size();
```

## 10) Examples

### Example A: Basic bounded pipeline

```cpp
#include <thread>
#include "channel/bounded_channel.h"

int main() {
    xtd::bounded_channel<int, 4> channel;

    std::thread producer([&] {
        auto writer = channel.writer();
        for (int i = 1; i <= 8; ++i) {
            writer.push(i);
        }
        writer.complete();
    });

    auto reader = channel.reader();
    int sum = 0;
    while (auto v = reader.read()) {
        sum += *v;
    }

    producer.join();
    return sum == 36 ? 0 : 1;
}
```

### Example B: Blocking consumer loop (preferred)

```cpp
#include "channel/unbounded_channel.h"

int main() {
    xtd::unbounded_channel<int> channel;
    auto writer = channel.writer();
    auto reader = channel.reader();

    writer.push(10);
    writer.push(20);
    writer.complete();

    int total = 0;
    while (auto v = reader.read()) {
        total += *v;
    }

    return total == 30 ? 0 : 1;
}
```

### Example C: Multiple producers, one concurrent consumer

```cpp
#include <thread>
#include <vector>
#include "channel/bounded_channel.h"

int main() {
    xtd::bounded_channel<int, 64> channel;
    auto completion_writer = channel.writer();
    std::size_t count = 0;
    std::thread consumer([&channel, &count] {
        auto reader = channel.reader();
        while (reader.read().has_value()) {
            ++count;
        }
    });

    std::vector<std::thread> producers;
    producers.reserve(4);
    for (int p = 0; p < 4; ++p) {
        producers.emplace_back([&channel, p] {
            auto writer = channel.writer();
            for (int i = 0; i < 100; ++i) {
                writer.push(p * 100 + i);
            }
        });
    }

    for (auto& t : producers) t.join();
    completion_writer.complete();
    consumer.join();

    return count == 400 ? 0 : 1;
}
```

## See also

- `tests/channels.cpp` for behavior-oriented usage tests.
- `src/channel/bounded_channel.h`
- `src/channel/unbounded_channel.h`
