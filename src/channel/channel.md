# xtd Channel Reference

This page documents the public channel API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Header and public types](#2-header-and-public-types)
3. [`xtd::channel<T>`](#3-xtdchannelt)
4. [`xtd::channel_writer<T>`](#4-xtdchannel_writert)
5. [`xtd::channel_reader<T>`](#5-xtdchannel_readert)
6. [Semantics and guarantees](#6-semantics-and-guarantees)
7. [Complexity](#7-complexity)
8. [How-to guides](#8-how-to-guides)
9. [Examples](#9-examples)
10. [Migration from the previous API](#10-migration-from-the-previous-api)

## 1) Overview

`xtd::channel<T>` is a thread-safe FIFO message queue for producer/consumer workloads.

A channel can operate in either mode:

- **Unbounded:** construct it without a capacity, or with capacity `0`.
- **Bounded:** construct it with a capacity greater than `0`.

Writers and readers are exposed through endpoint objects returned by `writer()` and `reader()`.

Primary use cases include:

- Multi-producer, multi-consumer task dispatch.
- Backpressure through bounded capacity.
- Graceful shutdown of consumer loops.
- Transfer of copyable or move-only values between threads.

## 2) Header and public types

### Namespace

```cpp
namespace xtd {
    // ...
}
```

### Header

```cpp
#include "channel/channel.h"
```

### Public types

```cpp
xtd::channel<T>
xtd::channel_writer<T>
xtd::channel_reader<T>
```

`channel_writer<T>` and `channel_reader<T>` are concrete, non-polymorphic endpoint types. They are owned by their `channel<T>` and returned by reference.

## 3) `xtd::channel<T>`

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel {
public:
    explicit channel(std::size_t capacity = 0);

    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;
    channel(channel&&) = delete;
    channel& operator=(channel&&) = delete;

    [[nodiscard]]
    channel_writer<T>& writer() noexcept;

    [[nodiscard]]
    channel_reader<T>& reader() noexcept;
};

}
```

### Template parameter

- `T`: message value type.

### Constructor

```cpp
explicit channel(std::size_t capacity = 0);
```

Creates a channel with the requested maximum buffered-item count.

| `capacity` | Mode | Writer behavior when the queue reaches capacity |
|---:|---|---|
| `0` | Unbounded | Writers do not wait for capacity |
| Greater than `0` | Bounded | Blocking writes wait until space becomes available or the channel completes |

The configured capacity cannot be changed after construction.

Examples:

```cpp
xtd::channel<int> unbounded;
xtd::channel<int> also_unbounded(0);
xtd::channel<int> bounded(128);
```

### `writer()`

```cpp
channel_writer<T>& writer() noexcept;
```

Returns a reference to the channel's write endpoint.

The returned endpoint is owned by the channel. It must not outlive the channel.

### `reader()`

```cpp
channel_reader<T>& reader() noexcept;
```

Returns a reference to the channel's read endpoint.

The returned endpoint is owned by the channel. It must not outlive the channel.

### Object lifetime and copying

`channel<T>` is neither copyable nor movable. Keep the channel at a stable address for as long as its reader and writer endpoints are in use.

## 4) `xtd::channel_writer<T>`

The write endpoint returned by `channel.writer()`.

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel_writer {
public:
    channel_writer(const channel_writer&) = delete;
    channel_writer& operator=(const channel_writer&) = delete;
    channel_writer(channel_writer&&) = delete;
    channel_writer& operator=(channel_writer&&) = delete;

    [[nodiscard]]
    bool push(const T& value);

    [[nodiscard]]
    bool push(T&& value);

    [[nodiscard]]
    bool try_push(const T& value);

    [[nodiscard]]
    bool try_push(T&& value);

    template<class... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]]
    bool emplace(Args&&... args);

    template<class... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]]
    bool try_emplace(Args&&... args);

    void complete();
};

}
```

### Acquiring the endpoint

Bind the returned endpoint by reference:

```cpp
xtd::channel<int> channel;
auto& writer = channel.writer();
```

An explicit endpoint type also remains supported:

```cpp
xtd::channel_writer<int>& writer = channel.writer();
```

Do not copy the endpoint:

```cpp
// Does not compile: channel_writer is non-copyable.
auto writer = channel.writer();
```

### `push`

```cpp
bool push(const T& value);
bool push(T&& value);
```

Attempts to enqueue a value.

- In unbounded mode, it does not wait for queue capacity.
- In bounded mode, it waits while the queue is full.
- If the channel completes before the value can be accepted, it returns `false`.
- Otherwise, it enqueues the value and returns `true`.

The lvalue overload copies the value. The rvalue overload moves it.

### `try_push`

```cpp
bool try_push(const T& value);
bool try_push(T&& value);
```

Attempts to enqueue a value without waiting for capacity.

Returns:

- `true` when the value is enqueued.
- `false` when the bounded channel is full.
- `false` when the channel is completed.

A `false` return value does not distinguish between a full channel and a completed channel.

### `emplace`

```cpp
template<class... Args>
bool emplace(Args&&... args);
```

Constructs a temporary `T` from `args...`, then passes that temporary to `push`.

It has the same blocking and completion behavior as `push`.

> This is not direct in-place construction inside the channel's queue storage.

### `try_emplace`

```cpp
template<class... Args>
bool try_emplace(Args&&... args);
```

Constructs a temporary `T` from `args...`, then passes that temporary to `try_push`.

It never waits for capacity.

### `complete`

```cpp
void complete();
```

Marks the channel as completed for writing.

After completion:

- New writes are rejected.
- Blocked writers wake and return `false`.
- Blocked readers wake.
- Values already buffered remain readable.
- Readers receive `std::nullopt` after the buffer is drained.

Calling `complete()` multiple times is supported.

## 5) `xtd::channel_reader<T>`

The read endpoint returned by `channel.reader()`.

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel_reader {
public:
    channel_reader(const channel_reader&) = delete;
    channel_reader& operator=(const channel_reader&) = delete;
    channel_reader(channel_reader&&) = delete;
    channel_reader& operator=(channel_reader&&) = delete;

    [[nodiscard]]
    std::optional<T> try_read();

    [[nodiscard]]
    std::optional<T> read();

    [[nodiscard]]
    std::size_t size() const;
};

}
```

### Acquiring the endpoint

Bind the returned endpoint by reference:

```cpp
xtd::channel<int> channel;
auto& reader = channel.reader();
```

An explicit endpoint type also remains supported:

```cpp
xtd::channel_reader<int>& reader = channel.reader();
```

Do not copy the endpoint:

```cpp
// Does not compile: channel_reader is non-copyable.
auto reader = channel.reader();
```

### `try_read`

```cpp
std::optional<T> try_read();
```

Attempts to remove and return the next value without waiting.

Returns:

- An engaged `std::optional<T>` when a value is available.
- `std::nullopt` when the queue is empty.

An empty result does not distinguish between an empty active channel and an empty completed channel.

### `read`

```cpp
std::optional<T> read();
```

Waits while the queue is empty and the channel is not completed.

Returns:

- An engaged `std::optional<T>` containing the next value.
- `std::nullopt` when the channel is completed and no buffered values remain.

### `size`

```cpp
std::size_t size() const;
```

Returns a synchronized snapshot of the current buffered-item count.

In concurrent code, the size may change immediately after the method returns. Do not use it as a substitute for `try_read()` or `try_push()` when making synchronization decisions.

## 6) Semantics and guarantees

### Thread safety

A channel supports concurrent use by multiple producers and multiple consumers.

The same writer or reader endpoint reference may be shared between threads, provided the owning channel remains alive.

### Ordering

Values are read in FIFO order according to their successful enqueue order.

When multiple producers write concurrently, their relative enqueue order depends on synchronization scheduling.

### Completion

Completion is cooperative:

1. A writer calls `complete()`.
2. Subsequent writes return `false`.
3. Buffered values remain available to readers.
4. Once the queue is empty, `read()` returns `std::nullopt`.

A typical consumer loop is:

```cpp
while (auto value = reader.read()) {
    // Process *value.
}
```

### Blocking behavior

| Operation | Unbounded mode | Bounded mode |
|---|---|---|
| `push`, `emplace` | Does not wait for capacity | Waits while full |
| `try_push`, `try_emplace` | Does not wait for capacity | Never waits; fails when full |
| `read` | Waits while empty | Waits while empty |
| `try_read` | Never waits | Never waits |

All operations may still briefly wait to acquire the channel's internal synchronization lock.

### Value transfer

- `push(lvalue)` copies.
- `push(rvalue)` moves.
- `emplace(args...)` constructs a temporary `T`, then moves it into the channel.
- `read()` and `try_read()` return `std::optional<T>` by value.

For move-only or expensive values, move the result out of the optional when taking ownership:

```cpp
if (auto item = reader.read()) {
    T value = std::move(*item);
}
```

### Exceptions

Operations that copy, move, construct, or allocate storage may propagate exceptions from `T` or the underlying storage implementation.

## 7) Complexity

Typical complexity, excluding blocking wait time and thread scheduling:

| Operation | Typical complexity |
|---|---:|
| `push`, `try_push` | O(1) |
| `emplace`, `try_emplace` | O(1), plus construction of `T` |
| `read`, `try_read` | O(1) |
| `size` | O(1) |
| `complete` | O(1), excluding the cost of waking blocked threads |

Memory usage grows with the number of queued values. In bounded mode, the number of buffered values is limited by the configured capacity.

## 8) How-to guides

### Create an unbounded producer/consumer pair

```cpp
xtd::channel<int> channel;

auto& writer = channel.writer();
auto& reader = channel.reader();
```

### Create a bounded producer/consumer pair

```cpp
xtd::channel<int> channel(128);

auto& writer = channel.writer();
auto& reader = channel.reader();
```

### Preserve explicit endpoint declarations

```cpp
xtd::channel<int> channel(128);

xtd::channel_writer<int>& writer = channel.writer();
xtd::channel_reader<int>& reader = channel.reader();
```

### Stop consumers cleanly

```cpp
// Producer side
writer.complete();

// Consumer side
while (auto value = reader.read()) {
    // Process *value.
}

// The loop exits when the channel is completed and drained.
```

### Avoid waiting when a bounded channel is full

```cpp
if (!writer.try_push(item)) {
    // The channel is full or completed.
    // Apply a retry, backoff, or drop policy.
}
```

### Transfer copyable values

```cpp
struct Job {
    std::vector<int> payload;
};

xtd::channel<Job> jobs;
auto& writer = jobs.writer();
auto& reader = jobs.reader();

Job first{{1, 2, 3}};

writer.push(first);             // Copies first.
writer.push(std::move(first));  // Moves first.
writer.emplace(std::vector<int>{4, 5, 6});

writer.complete();

while (auto item = reader.read()) {
    Job job = std::move(*item);
    // Process job.
}
```

### Transfer move-only values

```cpp
struct Job {
    std::vector<int> payload;
};

xtd::channel<std::unique_ptr<Job>> jobs;
auto& writer = jobs.writer();
auto& reader = jobs.reader();

writer.push(
    std::make_unique<Job>(
        Job{{1, 2, 3}}));

writer.complete();

while (auto item = reader.read()) {
    std::unique_ptr<Job> job = std::move(*item);
    // Process *job.
}
```

### Accumulate consumed values

```cpp
std::vector<Job> completed_jobs;

while (auto item = reader.read()) {
    completed_jobs.push_back(std::move(*item));
}
```

### Monitor the current backlog

```cpp
const std::size_t queued = reader.size();
```

Treat the result as an observation only; concurrent producers or consumers may change it immediately.

## 9) Examples

### Example A: Bounded pipeline

```cpp
#include "channel/channel.h"

#include <thread>

int main()
{
    xtd::channel<int> channel(4);

    auto& writer = channel.writer();
    auto& reader = channel.reader();

    std::thread producer(
        [&writer]
        {
            for (int i = 1; i <= 8; ++i) {
                writer.push(i);
            }

            writer.complete();
        });

    int sum = 0;

    while (auto value = reader.read()) {
        sum += *value;
    }

    producer.join();

    return sum == 36 ? 0 : 1;
}
```

### Example B: Unbounded channel

```cpp
#include "channel/channel.h"

int main()
{
    xtd::channel<int> channel;

    auto& writer = channel.writer();
    auto& reader = channel.reader();

    writer.push(10);
    writer.push(20);
    writer.complete();

    int total = 0;

    while (auto value = reader.read()) {
        total += *value;
    }

    return total == 30 ? 0 : 1;
}
```

### Example C: Multiple producers and one consumer

```cpp
#include "channel/channel.h"

#include <cstddef>
#include <thread>
#include <vector>

int main()
{
    xtd::channel<int> channel(64);

    auto& writer = channel.writer();
    auto& reader = channel.reader();

    std::size_t count = 0;

    std::thread consumer(
        [&reader, &count]
        {
            while (reader.read()) {
                ++count;
            }
        });

    std::vector<std::thread> producers;
    producers.reserve(4);

    for (int producer_id = 0; producer_id < 4; ++producer_id) {
        producers.emplace_back(
            [&writer, producer_id]
            {
                for (int i = 0; i < 100; ++i) {
                    writer.push(producer_id * 100 + i);
                }
            });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    writer.complete();
    consumer.join();

    return count == 400 ? 0 : 1;
}
```

## See also

- [`tests/channels.cpp`](../../tests/channels.cpp) for behavior-oriented tests.
- [`channel.h`](channel.h)
- [`channel_writer.h`](channel_writer.h)
- [`channel_reader.h`](channel_reader.h)