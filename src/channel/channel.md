# xtd Channel Reference

This page documents the public channel API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Header and public types](#2-header-and-public-types)
3. [`xtd::channel<T>`](#3-xtdchannelt)
4. [`xtd::channel_writer<T>`](#4-xtdchannel_writert)
5. [`xtd::channel_reader<T>`](#5-xtdchannel_readert)
6. [Cancellation with `std::stop_token`](#6-cancellation-with-stdstop_token)
7. [Semantics and guarantees](#7-semantics-and-guarantees)
8. [Complexity](#8-complexity)
9. [How-to guides](#9-how-to-guides)
10. [Examples](#10-examples)
11. [Migration notes](#11-migration-notes)

## 1) Overview

`xtd::channel<T>` is a thread-safe FIFO message queue for producer/consumer
workloads.

A channel can operate in either mode:

- **Unbounded:** construct it without a capacity, or with capacity `0`.
- **Bounded:** construct it with a capacity greater than `0`.

Writers and readers are exposed through endpoint objects returned by
`writer()` and `reader()`.

Primary use cases include:

- multi-producer, multi-consumer task dispatch;
- backpressure through bounded capacity;
- graceful producer completion and consumer shutdown;
- cancellable blocking waits through `std::stop_token`;
- transfer of copyable or move-only values between threads.

Cancellation and completion are separate concepts:

- a stop request cancels one blocking operation;
- `channel_writer<T>::complete()` permanently closes the channel for writing.

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

The cancellation overloads use the standard C++20 stop-token types:

```cpp
#include <stop_token>
```

### Public types

```cpp
xtd::channel<T>
xtd::channel_writer<T>
xtd::channel_reader<T>
```

`channel_writer<T>` and `channel_reader<T>` are concrete, non-polymorphic
endpoint types. They are owned by their `channel<T>` and returned by reference.

All three public objects are non-copyable and non-movable.

## 3) `xtd::channel<T>`

### Synopsis

```cpp
namespace xtd {

template<class T>
class channel {
public:
    explicit channel(std::size_t capacity = 0);
    ~channel();

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

`T` must support the construction required by the selected write overload.
Reading a value moves it out of the internal queue.

### Constructor

```cpp
explicit channel(std::size_t capacity = 0);
```

Creates a channel with the requested maximum buffered-item count.

| `capacity` | Mode | Writer behavior at capacity |
|---:|---|---|
| `0` | Unbounded | Writes do not wait for capacity |
| Greater than `0` | Bounded | Blocking writes wait until space, completion, or cancellation |

The configured capacity cannot be changed after construction.

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

The endpoint is owned by the channel and must not outlive it.

### `reader()`

```cpp
channel_reader<T>& reader() noexcept;
```

Returns a reference to the channel's read endpoint.

The endpoint is owned by the channel and must not outlive it.

### Destructor and lifetime

The channel destructor completes the writer side, waking blocked operations.

This does not make destruction safe while other threads are still accessing the
channel. All users of the writer and reader references must finish before the
owning channel object is destroyed.

Keep the channel at a stable address for the entire endpoint lifetime.

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
    bool push(std::stop_token stop_token, const T& value);

    [[nodiscard]]
    bool push(T&& value);

    [[nodiscard]]
    bool push(std::stop_token stop_token, T&& value);

    template<class... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]]
    bool emplace(Args&&... args);

    template<class... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]]
    bool emplace(std::stop_token stop_token, Args&&... args);

    [[nodiscard]]
    bool try_push(const T& value);

    [[nodiscard]]
    bool try_push(T&& value);

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

An explicit endpoint declaration is also supported:

```cpp
xtd::channel_writer<int>& writer = channel.writer();
```

### `push`

```cpp
bool push(const T& value);
bool push(std::stop_token stop_token, const T& value);

bool push(T&& value);
bool push(std::stop_token stop_token, T&& value);
```

Attempts to enqueue a value.

- In unbounded mode, no capacity wait is normally required.
- In bounded mode, the blocking overload waits while the queue is full.
- The function returns `true` after successfully enqueuing the value.
- It returns `false` if the channel completes before insertion.
- A token overload returns `false` if cancellation is observed before insertion.

The token is the **first argument**:

```cpp
writer.push(stop_token, value);
writer.push(stop_token, std::move(value));
```

The lvalue overload copies the value. The rvalue overload moves it only when the
queue insertion is actually performed.

In particular, a failed push does not consume an rvalue merely because it was
passed with `std::move`:

```cpp
writer.complete();

std::unique_ptr<Job> job = std::make_unique<Job>();
const bool accepted = writer.push(std::move(job));

// accepted == false
// job remains non-null because no queue insertion occurred.
```

### `try_push`

```cpp
bool try_push(const T& value);
bool try_push(T&& value);
```

Attempts to enqueue a value without waiting for capacity.

Returns:

- `true` when the value is enqueued;
- `false` when a bounded channel is full;
- `false` when the channel is completed.

A `false` result does not distinguish full capacity from completion.

`try_push` has no stop-token overload because it does not wait for queue state
to change.

### `emplace`

```cpp
template<class... Args>
bool emplace(Args&&... args);

template<class... Args>
bool emplace(std::stop_token stop_token, Args&&... args);
```

Constructs `T` directly in the channel's queue storage after the write is
allowed to proceed.

This is genuine in-place construction. The constructor is not called while a
non-blocking attempt is known to fail because the queue is full or completed.

The cancellable overload places the token first:

```cpp
writer.emplace(stop_token, constructor_arg_1, constructor_arg_2);
```

Returns `false` without constructing `T` when cancellation, completion, or a
failed wait prevents insertion.

### `try_emplace`

```cpp
template<class... Args>
bool try_emplace(Args&&... args);
```

Attempts direct in-place construction without waiting.

Returns `false` when the channel is full or completed. When it returns `false`,
the `T` constructor is not invoked.

### `complete`

```cpp
void complete();
```

Marks the channel as completed for writing.

After completion:

- new writes are rejected;
- blocked writers wake and return `false`;
- blocked readers wake;
- buffered values remain readable;
- readers receive `std::nullopt` after the queue is drained.

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
    std::optional<T> read(std::stop_token stop_token = {});

    [[nodiscard]]
    std::size_t size() const;
};

}
```

### `try_read`

```cpp
std::optional<T> try_read();
```

Attempts to remove and return the next value without waiting.

Returns:

- an engaged `std::optional<T>` when a value is available;
- `std::nullopt` when the queue is empty.

An empty result does not distinguish an empty active channel from an empty
completed channel.

### `read`

```cpp
std::optional<T> read(std::stop_token stop_token = {});
```

Waits while the queue is empty and the writer is not completed.

Returns:

- an engaged `std::optional<T>` containing the next value;
- `std::nullopt` when the channel is completed and drained;
- `std::nullopt` when cancellation is observed before a value is removed.

A default-constructed token has no stop state, so this remains valid:

```cpp
auto value = reader.read();
```

The implementation checks cancellation before removing a queued value. An
already-stopped token therefore returns `std::nullopt` immediately, even if the
operation would otherwise not need to block.

Because cancellation and completed-and-drained both return `std::nullopt`,
callers that need to distinguish them should also inspect their token:

```cpp
auto value = reader.read(stop_token);

if (!value) {
    if (stop_token.stop_requested()) {
        // Cancelled.
    } else {
        // Writer completed and queue drained.
    }
}
```

### `size`

```cpp
std::size_t size() const;
```

Returns a synchronized snapshot of the current buffered-item count.

The size can change immediately after return. Do not use it as a substitute for
`try_read()` or `try_push()` when making synchronization decisions.

## 6) Cancellation with `std::stop_token`

Cancellation is cooperative and scoped to the current blocking call.

Supported cancellable operations:

| Endpoint | Operation |
|---|---|
| Writer | `push(stop_token, value)` |
| Writer | `emplace(stop_token, args...)` |
| Reader | `read(stop_token)` |

Non-blocking operations do not accept a token.

A stop request:

- wakes a blocked channel wait;
- causes the operation to return `false` or `std::nullopt`;
- does not complete the channel;
- does not discard buffered messages;
- does not affect future operations using another token.

An already-requested stop cancels immediately.

### Cancellation of a bounded write

```cpp
xtd::channel<int> channel(1);
auto& writer = channel.writer();

writer.push(1);

std::stop_source source;

// Blocks while the channel is full, unless stop is requested.
const bool inserted = writer.push(source.get_token(), 2);
```

If the call is cancelled, `2` is not inserted.

### Cancellation of a read

```cpp
std::stop_source source;
auto& reader = channel.reader();

const std::optional<int> value = reader.read(source.get_token());
```

If cancelled before removal, the queue remains unchanged.

### `std::jthread`

```cpp
std::jthread consumer([&](std::stop_token stop_token) {
    auto& reader = channel.reader();

    while (!stop_token.stop_requested()) {
        auto item = reader.read(stop_token);

        if (!item) {
            break;
        }

        process(std::move(*item));
    }
});
```

The producer should still call `complete()` when it has permanently finished
writing.

## 7) Semantics and guarantees

### Thread safety

A channel supports concurrent use by multiple producers and multiple consumers.

The same writer or reader reference may be shared between threads, provided the
owning channel remains alive.

### Ordering

Values are read in FIFO order according to successful enqueue order.

When multiple producers write concurrently, their relative enqueue order
depends on lock acquisition and scheduling.

### Completion

Completion is cooperative:

1. a writer calls `complete()`;
2. later writes return `false`;
3. buffered values remain available;
4. after the queue is drained, `read()` returns `std::nullopt`.

```cpp
while (auto value = reader.read()) {
    process(*value);
}
```

### Cancellation versus completion

| Property | Cancellation | Completion |
|---|---|---|
| Scope | One operation/token | Entire writer endpoint |
| Future writes | Allowed | Rejected |
| Buffered values | Preserved | Preserved |
| Blocked operations wake | Yes | Yes |
| Consumer can distinguish from return alone | No | No |

### Blocking behavior

| Operation | Unbounded mode | Bounded mode | Stop-token support |
|---|---|---|---|
| `push`, `emplace` | No capacity wait | Waits while full | Yes |
| `try_push`, `try_emplace` | Never waits for capacity | Never waits; fails when full | No |
| `read` | Waits while empty | Waits while empty | Yes |
| `try_read` | Never waits for data | Never waits for data | No |

Operations may briefly wait to acquire the internal mutex even when described
as non-blocking.

### Wake-up behavior

The implementation tracks waiting readers and writers:

- a successful enqueue can notify one waiting reader;
- a successful dequeue can notify one waiting writer;
- completion notifies all blocked readers and writers;
- a stop request wakes the wait registered with that token.

No fairness guarantee is provided.

### Value transfer

- `push(lvalue)` copies;
- `push(rvalue)` moves when insertion succeeds;
- `emplace(args...)` constructs directly in the queue;
- `read()` and `try_read()` move the front value into an optional result.

For move-only or expensive values:

```cpp
if (auto item = reader.read()) {
    T value = std::move(*item);
}
```

### Exceptions

Operations may propagate exceptions from:

- construction, copying, or moving of `T`;
- allocation performed by the internal queue;
- user-defined constructors used by `emplace`.

Completion and cancellation themselves are represented by return values rather
than exceptions.

## 8) Complexity

Typical complexity, excluding wait time, scheduling, and allocation details:

| Operation | Typical complexity |
|---|---:|
| `push`, `try_push` | O(1) |
| `emplace`, `try_emplace` | O(1), plus construction of `T` |
| `read`, `try_read` | O(1) |
| `size` | O(1) |
| `complete` | O(1), excluding wake-up scheduling |

Memory usage grows with queued values. In bounded mode, the buffered-item count
does not exceed the configured capacity.

## 9) How-to guides

### Create an unbounded producer/consumer pair

```cpp
xtd::channel<int> channel;

auto& writer = channel.writer();
auto& reader = channel.reader();
```

### Create a bounded pair with backpressure

```cpp
xtd::channel<int> channel(128);

auto& writer = channel.writer();
auto& reader = channel.reader();
```

### Stop consumers cleanly with completion

```cpp
writer.complete();

while (auto value = reader.read()) {
    process(*value);
}
```

### Cancel a blocked consumer without completing the channel

```cpp
std::stop_source source;

auto value = reader.read(source.get_token());

source.request_stop();
```

A later read with another token can still use the channel.

### Avoid waiting when full

```cpp
if (!writer.try_push(item)) {
    // Full or completed.
    // Apply retry, backoff, or drop policy.
}
```

### Transfer move-only values

```cpp
xtd::channel<std::unique_ptr<Job>> jobs;

auto& writer = jobs.writer();
auto& reader = jobs.reader();

writer.push(std::make_unique<Job>());
writer.complete();

while (auto item = reader.read()) {
    std::unique_ptr<Job> job = std::move(*item);
    process(*job);
}
```

### Construct directly in queue storage

```cpp
struct Job {
    Job(int id, std::string payload);
};

writer.emplace(42, "payload");
writer.emplace(stop_token, 43, "cancellable");
```

### Observe backlog

```cpp
const std::size_t queued = reader.size();
```

Treat the value as monitoring information only.

## 10) Examples

### Bounded producer and consumer

```cpp
#include "channel/channel.h"

#include <thread>

int main()
{
    xtd::channel<int> channel(4);

    auto& writer = channel.writer();
    auto& reader = channel.reader();

    std::thread producer([&writer] {
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

### Cancellable bounded producer

```cpp
#include "channel/channel.h"

#include <stop_token>
#include <thread>

xtd::channel<Work> work(64);
auto& writer = work.writer();

std::jthread producer([&](std::stop_token stop_token) {
    for (Work item : make_work()) {
        if (!writer.push(stop_token, std::move(item))) {
            break;
        }
    }

    writer.complete();
});
```

### Multiple producers and one consumer

```cpp
xtd::channel<int> channel(64);

auto& writer = channel.writer();
auto& reader = channel.reader();

std::thread consumer([&] {
    while (auto item = reader.read()) {
        process(*item);
    }
});

std::vector<std::thread> producers;

for (int producer_id = 0; producer_id < 4; ++producer_id) {
    producers.emplace_back([&, producer_id] {
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
```

## 11) Migration notes

### Cancellable overload order

The channel writer API uses token-first overloads:

```cpp
writer.push(stop_token, value);
writer.emplace(stop_token, args...);
```

It does **not** use:

```cpp
writer.push(value, stop_token); // not the public signature
```

### `emplace` behavior

`emplace` now constructs directly in the internal queue after capacity,
completion, and cancellation checks. Documentation that describes creation of a
temporary followed by `push` is outdated.

### Cancellation return values

No new result type was introduced:

- cancelled writes return `false`;
- cancelled reads return `std::nullopt`.

Use `stop_token.stop_requested()` when the reason must be distinguished from
completion or capacity-related failure.

## See also

- [`tests/channels.cpp`](../../tests/channels.cpp)
- [`channel.h`](channel.h)
- [`channel_writer.h`](channel_writer.h)
- [`channel_reader.h`](channel_reader.h)