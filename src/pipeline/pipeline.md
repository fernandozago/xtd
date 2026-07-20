# xtd Pipeline Reference

This page documents the public pipeline API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Headers and public types](#2-headers-and-public-types)
3. [`xtd::pipe_options`](#3-xtdpipe_options)
4. [`xtd::pipeline`](#4-xtdpipeline)
5. [`xtd::pipe_writer`](#5-xtdpipe_writer)
6. [`xtd::pipe_reader`](#6-xtdpipe_reader)
7. [Cancellation with `std::stop_token`](#7-cancellation-with-stdstop_token)
8. [`xtd::read_result`](#8-xtdread_result)
9. [`xtd::segmented_byte_view`](#9-xtdsegmented_byte_view)
10. [`xtd::position`](#10-xtdposition)
11. [`xtd::pipe_utils`](#11-xtdpipe_utils)
12. [Semantics and guarantees](#12-semantics-and-guarantees)
13. [Complexity](#13-complexity)
14. [How-to guides](#14-how-to-guides)
15. [Examples](#15-examples)
16. [Migration notes](#16-migration-notes)

## 1) Overview

`xtd::pipeline` is a byte-oriented producer/consumer primitive for incremental
parsing.

Core model:

- the writer appends bytes through `pipe_writer::write(...)`;
- the reader obtains snapshots through `read()` or `read_at_least(...)`;
- each successful read must be matched by exactly one `advance(...)`;
- bounded memory growth is controlled by pause/resume thresholds;
- blocking reads and writes support cooperative cancellation.

Typical use cases:

- delimiter-based protocol parsing;
- incremental binary framing;
- streaming file or socket ingestion;
- producer/consumer byte transfer with backpressure;
- cancellation-aware `std::jthread` processing.

The pipeline is designed for one logical writer and one logical reader, which
may run concurrently.

## 2) Headers and public types

```cpp
#include "pipeline/pipeline.h"
#include "pipeline/pipe_utils.h"
```

Cancellation support uses:

```cpp
#include <stop_token>
```

Main public types:

```cpp
xtd::pipe_options
xtd::pipeline
xtd::pipe_writer
xtd::pipe_reader
xtd::read_result
xtd::segmented_byte_view
xtd::position
xtd::pipe_utils
```

## 3) `xtd::pipe_options`

### Synopsis

```cpp
namespace xtd {

struct pipe_options {
    std::size_t buffer_size = 1024 * 4;
    std::size_t resume_writer_threshold = 1024 * 32;
    std::size_t pause_writer_threshold = 1024 * 128;
};

}
```

### Members

| Member | Meaning |
|---|---|
| `buffer_size` | Capacity of each internal data segment |
| `resume_writer_threshold` | Buffered-byte level at which a paused writer may resume |
| `pause_writer_threshold` | Buffered-byte level at which the writer pauses |

### Validation

Construction throws `std::invalid_argument` unless:

- `buffer_size > 0`;
- `pause_writer_threshold > 0`;
- `resume_writer_threshold <= pause_writer_threshold`.

### Current threshold policy

The current implementation bases writer resumption only on the number of
**unconsumed buffered bytes**:

```cpp
writer_resumes =
    buffered_bytes <= resume_writer_threshold;
```

Examining bytes without consuming them does not release storage and therefore
does not resume the writer.

The pause and resume thresholds should be large enough to hold the largest
application-level frame that may need to remain buffered while parsing.

### Large writes

A large write is incremental rather than atomic:

1. bytes are copied into one or more internal segments;
2. once `pause_writer_threshold` is reached, the writer pauses;
3. the same `write()` call waits for reader consumption;
4. it resumes when buffered bytes fall to the resume threshold;
5. it continues until complete, cancelled, or failed.

A successful `write()` can therefore block in the middle of its input.

## 4) `xtd::pipeline`

### Synopsis

```cpp
namespace xtd {

class pipeline {
public:
    explicit pipeline(pipe_options options = {});
    ~pipeline();

    pipeline(const pipeline&) = delete;
    pipeline& operator=(const pipeline&) = delete;
    pipeline(pipeline&&) = delete;
    pipeline& operator=(pipeline&&) = delete;

    [[nodiscard]]
    pipe_reader& reader() noexcept;

    [[nodiscard]]
    pipe_writer& writer() noexcept;
};

}
```

### Construction

```cpp
xtd::pipeline pipe;

xtd::pipeline tuned({
    .buffer_size = 4096,
    .resume_writer_threshold = 32768,
    .pause_writer_threshold = 131072,
});
```

The pipeline creates and owns one reader endpoint and one writer endpoint.

### Lifetime

The endpoints are returned by reference and must not outlive the pipeline.

The pipeline destructor completes both sides. Threads using the endpoints must
still stop and join before destruction; destructor completion is not a lifetime
synchronization mechanism.

### Internal segment pooling

The implementation stores data in fixed-size segments and maintains a pool
sized from `buffer_size` and `pause_writer_threshold`.

Snapshots are exposed as segmented views, avoiding the need to flatten all
buffered data into one contiguous allocation.

## 5) `xtd::pipe_writer`

### Synopsis

```cpp
namespace xtd {

class pipe_writer {
public:
    std::size_t write(
        const std::byte* data,
        std::size_t length,
        std::stop_token stop_token = {});

    template<class T>
        requires (
            std::convertible_to<T, std::string_view> ||
            std::is_trivially_copyable_v<T>)
    std::size_t write(
        const T& value,
        std::stop_token stop_token = {});

    void complete();
};

}
```

The endpoint is non-copyable and non-movable.

### `write(const std::byte*, std::size_t, std::stop_token)`

```cpp
std::size_t write(
    const std::byte* data,
    std::size_t length,
    std::stop_token stop_token = {});
```

Writes raw bytes.

Behavior:

- `length == 0` returns `0`;
- `data == nullptr` returns `0`, including for non-zero length;
- writing after writer completion throws `std::runtime_error`;
- writing after reader completion throws `std::runtime_error`;
- the call may block due to backpressure;
- successful completion returns `length`;
- cancellation returns the number of bytes already committed.

A cancelled write is not rolled back:

```cpp
const std::size_t written =
    writer.write(data, length, stop_token);

// 0 <= written && written <= length
```

When `written < length`, the prefix `[data, data + written)` may already be
visible to the reader.

### Typed `write`

```cpp
template<class T>
std::size_t write(
    const T& value,
    std::stop_token stop_token = {});
```

The overload participates only when `T` is string-view-convertible or trivially
copyable.

#### String-like values

For `T` convertible to `std::string_view`, the string's byte sequence is
written:

```cpp
writer.write("abc");                 // 3 bytes
writer.write(std::string{"abc"});    // 3 bytes
writer.write(std::string_view{"abc"});
```

#### Trivially copyable values

Otherwise, the native object representation is written:

```cpp
writer.write(std::uint32_t{42});
writer.write(std::array{1, 2, 3});
```

This is not portable serialization:

- endianness may differ;
- object layout and ABI may differ;
- padding bytes are included;
- persistent or network protocols should encode fields explicitly.

Cancellation can produce a partial object representation. Applications that
require atomic records should frame or stage records above the pipeline.

### `complete`

```cpp
void complete();
```

Marks the writer completed and wakes blocked readers and writers.

Properties:

- idempotent;
- buffered bytes remain readable;
- later `write(...)` calls throw `std::runtime_error`;
- completion does not imply that buffered data ends on an application message
  boundary.

## 6) `xtd::pipe_reader`

### Synopsis

```cpp
namespace xtd {

class pipe_reader {
public:
    [[nodiscard]]
    read_result read(std::stop_token stop_token = {}) const;

    [[nodiscard]]
    read_result read_at_least(
        std::size_t min_bytes,
        std::stop_token stop_token = {}) const;

    void advance(
        const position& consumed,
        const position& examined);

    void advance(const position& consumed);
    void advance(const segmented_byte_view& sequence);

    void complete();
};

}
```

The endpoint is non-copyable and non-movable.

### `read`

```cpp
read_result read(std::stop_token stop_token = {}) const;
```

Equivalent to:

```cpp
reader.read_at_least(0, stop_token);
```

The call waits until:

- new unexamined data is available;
- the writer is completed;
- the reader is completed; or
- cancellation is requested.

A successful read creates a pending-read state. Exactly one matching
`advance(...)` is required before the next read.

The call throws `std::runtime_error` when:

- the reader was completed;
- a previous successful read has not been advanced.

A cancelled read returns a cancelled `read_result`; it does not create a pending
read and does not require `advance(...)`.

### `read_at_least`

```cpp
read_result read_at_least(
    std::size_t min_bytes,
    std::stop_token stop_token = {}) const;
```

Waits until one of these conditions is true:

- the writer is completed;
- the reader is completed;
- at least `min_bytes` are buffered and some data remains unexamined;
- cancellation is requested.

Writer completion may return fewer than `min_bytes`.

The minimum is a wake/read condition, not a guarantee after completion.

### `advance(consumed, examined)`

```cpp
void advance(
    const position& consumed,
    const position& examined);
```

Reports progress for the most recent successful read.

- `consumed`: bytes before this position may be discarded and recycled.
- `examined`: bytes before this position were inspected by the parser.

Requirements:

- one successful read is pending;
- `consumed <= examined`;
- both positions belong to the most recent read sequence;
- `examined` does not exceed the pending read length;
- the reader is not completed.

Errors:

- protocol-state violations throw `std::runtime_error`;
- foreign, stale, reversed, or out-of-range positions throw
  `std::invalid_argument`.

Consuming data reduces `buffered_size` and can resume a paused writer.

Examining without consuming changes when the next read becomes eligible, but
does not free storage or independently resume the writer.

### Convenience overloads

```cpp
reader.advance(consumed);
```

Equivalent to:

```cpp
reader.advance(consumed, consumed);
```

And:

```cpp
reader.advance(sequence);
```

Equivalent to:

```cpp
reader.advance(sequence.begin(), sequence.end());
```

This consumes up to `sequence.begin()` and marks through `sequence.end()` as
examined.

### `complete`

```cpp
void complete();
```

Completes the reader.

Effects:

- clears buffered bytes;
- clears pending-read state;
- clears writer pause state;
- wakes blocked readers and writers;
- later reads and advances throw `std::runtime_error`;
- idempotent.

Completing the reader intentionally discards unread data.

## 7) Cancellation with `std::stop_token`

Supported blocking operations:

| Endpoint | Operation | Cancellation result |
|---|---|---|
| Writer | `write(..., stop_token)` | Number of bytes already written |
| Reader | `read(stop_token)` | Cancelled/false `read_result` |
| Reader | `read_at_least(n, stop_token)` | Cancelled/false `read_result` |

A default-constructed token preserves non-cancellable behavior.

A stop request:

- wakes the corresponding `condition_variable_any` wait;
- does not complete either endpoint;
- does not discard committed bytes;
- does not undo a previous successful read;
- does not create a pending read when cancellation occurs before a snapshot.

### Detecting a cancelled read

`read_result` has an explicit Boolean conversion:

```cpp
xtd::read_result result = reader.read(stop_token);

if (!result) {
    // The read was cancelled.
}
```

Cancellation is directly represented by `operator bool()`, unlike channel
reads, which use `std::nullopt` for both cancellation and completion.

### Already-stopped tokens

```cpp
std::stop_source source;
source.request_stop();

xtd::read_result result = reader.read(source.get_token());
// !result

std::size_t written =
    writer.write("payload", source.get_token());
// written == 0
```

### Partial write policy

A caller must handle partial writes explicitly:

```cpp
const std::size_t written =
    writer.write(frame.data(), frame.size(), stop_token);

if (written != frame.size()) {
    // A frame prefix may already be in the stream.
    // Decide whether to complete, abort the protocol, or discard the pipeline.
}
```

Cancellation does not provide transactional framing.

### Pending read interaction

Cancellation only applies while obtaining a new snapshot.

Once a successful `read_result` exists, the caller must still call
`advance(...)`, even if its token is later stopped.

## 8) `xtd::read_result`

### Synopsis

```cpp
namespace xtd {

struct read_result {
    read_result() = delete;

    explicit constexpr
    operator bool() const noexcept;

    [[nodiscard]]
    const segmented_byte_view& buffer() const noexcept;

    [[nodiscard]]
    bool completed() const noexcept;
};

}
```

The current type exposes a non-owning buffer reference and cancellation state.

### `operator bool`

```cpp
explicit operator bool() const noexcept;
```

Returns:

- `true` for a normal read result;
- `false` for a cancelled read.

### `buffer`

```cpp
const segmented_byte_view& buffer() const noexcept;
```

Returns the read snapshot.

The view is non-owning and references pipeline-managed storage.

### `completed`

```cpp
bool completed() const noexcept;
```

Reports the writer-completed state captured when the read result was created.

`completed()` can be `true` while `buffer()` is non-empty.

### Buffer lifetime

The buffer, its slices, spans, positions, and pointers remain valid only until:

- the matching `advance(...)`;
- reader completion;
- pipeline destruction.

Do not retain them after advancing.

## 9) `xtd::segmented_byte_view`

Represents a logical byte range backed by zero or more segments.

### Key members

```cpp
position begin() const noexcept;
position end() const noexcept;

std::size_t size() const noexcept;
bool empty() const noexcept;
std::size_t segment_count() const noexcept;

std::span<const std::span<const std::byte>>
segments() const noexcept;

segmented_byte_view slice(const position& end) const;

segmented_byte_view slice(
    const position& begin,
    const position& end) const;

segmented_byte_view slice(
    std::size_t begin_offset,
    const position& end) const;

segmented_byte_view slice(
    std::size_t begin_offset,
    std::size_t size) const;

position position_of(std::byte value) const;
position position_of(char value) const;

std::size_t copy_to(
    std::span<std::byte> destination) const noexcept;

std::size_t copy_to(
    std::byte* destination,
    std::size_t destination_size) const;

std::size_t copy_to(
    std::vector<std::byte>& destination) const noexcept;

template<class T>
    requires (
        std::is_trivially_copyable_v<T> &&
        !std::is_convertible_v<T, std::string_view>)
bool copy_to(T& destination) const;

std::string to_string() const;
```

### Segment access

`segments()` exposes the currently sliced spans. It does not copy payload bytes.

The returned span is subject to the same lifetime as the read snapshot.

### Slicing

Position-based slices use absolute offsets and require matching sequence
identity.

Offset-based slices are relative to the current view's beginning.

Invalid ranges throw `std::out_of_range`. Foreign positions throw
`std::invalid_argument`.

### Searching

```cpp
xtd::position newline = sequence.position_of('\n');
```

Returns an invalid/default position when no match exists:

```cpp
if (xtd::position newline = sequence.position_of('\n')) {
    // Found.
}
```

Search works across segment boundaries.

### Copying

Pointer/span/vector copies truncate to the smaller of source and destination
sizes.

```cpp
std::vector<std::byte> destination(128);
const std::size_t copied = sequence.copy_to(destination);
```

Typed copying requires enough source bytes for `sizeof(T)` and copies exactly
the destination object size.

### `to_string`

Copies the complete logical view into a `std::string`, preserving segment order.

## 10) `xtd::position`

A lightweight cursor associated with one segmented sequence identity.

Typical operations include:

```cpp
explicit operator bool() const noexcept;

position operator+(std::size_t offset) const noexcept;
position& operator+=(std::size_t offset) noexcept;

position& operator++() noexcept;
position operator++(int) noexcept;

bool operator==(const position&) const noexcept;
```

Position arithmetic:

- is forward-only;
- is `noexcept`;
- does not clamp;
- does not validate against a sequence end;
- uses unsigned `std::size_t` arithmetic.

A position can therefore be constructed past the end. Later slicing or
advancing rejects it.

Positions from older reads are stale after `advance(...)`.

## 11) `xtd::pipe_utils`

### Synopsis

```cpp
namespace xtd {

struct pipe_utils {
    static std::thread threaded_copy_file_from_path(
        const std::string& path,
        pipe_writer& writer,
        std::size_t chunk_size = 4096);

    static std::thread threaded_copy_from_socket(
        int socket_fd,
        pipe_writer& writer,
        std::size_t chunk_size = 4096);
};

}
```

### File copy

Starts a joinable thread that reads a file in chunks, writes each chunk to the
pipeline, and completes the writer before exiting.

Throws before starting the thread when:

- `chunk_size == 0`;
- the file cannot be opened by the initial probe.

The worker completes the writer even if reopening the file fails.

### Socket copy

Starts a joinable thread that receives from a socket and writes to the pipeline.

Validation:

- negative descriptors throw `std::invalid_argument`;
- zero chunk size throws `std::invalid_argument`.

The receive loop:

- continues after `EINTR`;
- stops on EOF;
- stops on other socket errors;
- completes the writer before thread exit.

### Cancellation limitation

The current `pipe_utils` helpers do not accept a `std::stop_token`.

They use the non-cancellable writer overload and can therefore remain blocked by
pipeline backpressure until reader progress or endpoint completion occurs.

Applications requiring cancellable file/socket ingestion should build their own
`std::jthread` loop and pass its token to `writer.write(...)`.

## 12) Semantics and guarantees

### Read/advance protocol

For each successful read:

1. inspect `result.buffer()`;
2. choose consumed and examined positions;
3. call exactly one `advance(...)`;
4. perform the next read.

A cancelled result is not a successful read and requires no advance.

Calling another read while one successful read remains pending throws.

### Data availability and examined bytes

A read becomes eligible when:

- the writer or reader completes; or
- data exists beyond the previously examined prefix and the minimum-size
  requirement is met.

Consequences:

- consume none, examine none: the same bytes can be returned again;
- consume none, examine all: the next read waits for a data-size/state change;
- consume bytes: storage is released and a paused writer may resume.

### Backpressure

The writer pauses at:

```cpp
buffered_size >= pause_writer_threshold
```

It resumes at:

```cpp
buffered_size <= resume_writer_threshold
```

Only consumption reduces `buffered_size`.

A single write may cross multiple pause/resume cycles.

### Completion

Writer completion:

- preserves buffered bytes;
- allows the reader to observe `completed() == true`;
- makes future writes fail.

Reader completion:

- discards buffered bytes;
- invalidates pending state;
- makes future reads and advances fail;
- wakes blocked writers.

### Cancellation

Cancellation:

- is operation-scoped;
- preserves endpoint state;
- preserves bytes already written;
- returns a false read result or partial write count.

### Thread safety

Supported model:

- one logical writer;
- one logical reader;
- writer and reader may run concurrently.

Writer operations are internally synchronized with each other.

Reader operations are internally synchronized, but the read/advance protocol is
single-consumer. Do not concurrently:

- call `read()` from multiple threads;
- advance while another thread processes the pending snapshot;
- reuse positions from another snapshot.

No fairness guarantee is provided.

### Notifications

A writer notifies a waiting reader after appending data.

Reader advancement notifies waiting readers and can notify one paused writer
when consumption crosses the resume threshold.

Completion notifies all relevant waits.

## 13) Complexity

Typical costs, excluding blocking and allocation:

| Operation | Typical complexity |
|---|---:|
| Append bytes | O(number of copied bytes) |
| Build read snapshot | O(number of readable segments) |
| Consume bytes | O(number of removed/partially advanced segments) |
| `position_of` | O(view size) |
| `copy_to` | O(number of copied bytes) |
| `to_string` | O(view size) |
| Position arithmetic | O(1) |

The segment pool limits repeated allocations during steady-state use, but the
exact allocation behavior depends on thresholds and workload.

## 14) How-to guides

### Configure backpressure

```cpp
xtd::pipeline pipe({
    .buffer_size = 4096,
    .resume_writer_threshold = 32 * 1024,
    .pause_writer_threshold = 128 * 1024,
});
```

Choose a pause threshold large enough for the largest frame that may remain
unconsumed during parsing.

### Consume an entire snapshot

```cpp
xtd::read_result result = reader.read();

if (result) {
    const auto& buffer = result.buffer();
    process(buffer);
    reader.advance(buffer.end());
}
```

### Retain an incomplete suffix

```cpp
auto result = reader.read();
const auto& buffer = result.buffer();

xtd::position delimiter = buffer.position_of('\n');

if (delimiter) {
    process(buffer.slice(buffer.begin(), delimiter));
    reader.advance(delimiter + 1, buffer.end());
} else {
    reader.advance(buffer.begin(), buffer.end());
}
```

This consumes nothing and examines all data when no delimiter is found.

### Read a minimum number of bytes

```cpp
auto result = reader.read_at_least(header_size, stop_token);

if (!result) {
    // Cancelled.
} else if (result.buffer().size() >= header_size) {
    parse_header(result.buffer());
}
```

Writer completion can still produce fewer bytes.

### Cancel a blocked write

```cpp
const std::size_t written =
    writer.write(payload.data(), payload.size(), stop_token);

if (written != payload.size()) {
    handle_partial_stream(written);
}
```

### Copy a view into a struct

```cpp
Header header{};

if (buffer.copy_to(header)) {
    // Native representation was copied.
}
```

Use explicit serialization for portable protocols.

## 15) Examples

### Cancellable reader loop

```cpp
#include "pipeline/pipeline.h"

#include <stop_token>
#include <thread>

xtd::pipeline pipe;

std::jthread parser([&](std::stop_token stop_token) {
    auto& reader = pipe.reader();

    while (true) {
        xtd::read_result result = reader.read(stop_token);

        if (!result) {
            break;
        }

        const xtd::segmented_byte_view& buffer = result.buffer();

        parse(buffer);
        reader.advance(buffer.end(), buffer.end());

        if (result.completed()) {
            break;
        }
    }
});
```

### Large cancellable writer

```cpp
std::jthread producer([&](std::stop_token stop_token) {
    auto& writer = pipe.writer();

    const std::size_t written =
        writer.write(payload.data(), payload.size(), stop_token);

    if (written == payload.size()) {
        writer.complete();
    } else {
        // The reader may already have seen the written prefix.
        handle_cancelled_write(written);
        writer.complete();
    }
});
```

### Delimiter parser retaining an incomplete suffix

```cpp
while (xtd::read_result result = reader.read(stop_token)) {
    const auto& buffer = result.buffer();
    xtd::segmented_byte_view remaining = buffer;

    while (xtd::position newline = remaining.position_of('\n')) {
        process_line(
            remaining.slice(remaining.begin(), newline));

        remaining =
            remaining.slice(newline + 1, remaining.end());
    }

    reader.advance(remaining.begin(), remaining.end());

    if (result.completed()) {
        if (!remaining.empty()) {
            handle_incomplete_final_line(remaining);
        }

        break;
    }
}
```

### Cancellable file ingestion without `pipe_utils`

```cpp
std::jthread producer([&](std::stop_token stop_token) {
    std::ifstream input(path, std::ios::binary);
    std::vector<std::byte> buffer(4096);

    while (!stop_token.stop_requested() && input) {
        input.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size()));

        const std::size_t count =
            static_cast<std::size_t>(input.gcount());

        if (count == 0) {
            break;
        }

        const std::size_t written =
            writer.write(buffer.data(), count, stop_token);

        if (written != count) {
            break;
        }
    }

    writer.complete();
});
```

## 16) Migration notes

### All blocking pipeline operations now accept a token

```cpp
reader.read(stop_token);
reader.read_at_least(min_bytes, stop_token);
writer.write(data, length, stop_token);
writer.write(value, stop_token);
```

The token is the last argument in the pipeline API.

This differs from channel writer overloads, where the token is first.

### `read_result` now represents cancellation

Use:

```cpp
if (!result) {
    // Cancelled.
}
```

Only successful results establish pending-read state.

### Writes can return partial counts

Previously, code may have assumed a successful return always equaled the input
size or that cancellation could not interrupt the call.

With cancellation, always compare the return value with the requested byte
count.

### Backpressure documentation change

Documentation that allows examined bytes alone to resume the writer is outdated.

The current code resumes based only on unconsumed buffered bytes:

```cpp
buffered_size <= resume_writer_threshold
```

This prevents unbounded growth when a parser repeatedly examines data without
consuming it.

### Null raw pointer behavior

The current implementation returns `0` when `data == nullptr`, including when
`length > 0`. It does not throw `std::invalid_argument` for that case.

## See also

- [`tests/pipelines.cpp`](../../tests/pipelines.cpp)
- [`pipeline.h`](pipeline.h)
- [`pipe_writer.h`](pipe_writer.h)
- [`pipe_reader.h`](pipe_reader.h)
- [`read_result.h`](read_result.h)
- [`segmented_byte_view.h`](segmented_byte_view.h)
- [`position.h`](position.h)
- [`pipe_utils.h`](pipe_utils.h)