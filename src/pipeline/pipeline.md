# xtd Pipeline Reference

This page documents the public pipeline API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Headers and Types](#2-headers-and-types)
3. [`xtd::pipe_options`](#3-xtdpipe_options)
4. [`xtd::pipeline`](#4-xtdpipeline)
5. [`xtd::pipe_writer`](#5-xtdpipe_writer)
6. [`xtd::pipe_reader`](#6-xtdpipe_reader)
7. [`xtd::read_result`](#7-xtdread_result)
8. [`xtd::segmented_byte_view`](#8-xtdsegmented_byte_view)
9. [`xtd::position`](#9-xtdposition)
10. [`xtd::pipe_utils`](#10-xtdpipe_utils)
11. [Semantics and Guarantees](#11-semantics-and-guarantees)
12. [Complexity](#12-complexity)
13. [How-to Guides](#13-how-to-guides)
14. [Examples](#14-examples)

## 1) Overview

`xtd::pipeline` is a byte-oriented producer/consumer primitive for incremental parsing.

Core model:

- Writer appends bytes via `pipe_writer::write(...)`.
- Reader receives snapshots via `pipe_reader::read()`.
- Reader must call `advance(...)` exactly once per successful `read()` before calling `read()` again.
- Backpressure is controlled by `pipe_options` thresholds.

Typical use cases:

- Delimiter-based protocol parsing.
- Incremental binary framing.
- Streaming file/socket ingestion while controlling memory growth.

## 2) Headers and Types

### Namespace

```cpp
namespace xtd { /* ... */ }
```

### Main headers

```cpp
#include "pipeline/pipeline.h"
#include "pipeline/pipe_utils.h"
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

### Validation rules

- `buffer_size > 0`
- `pause_writer_threshold > 0`
- `resume_writer_threshold <= pause_writer_threshold`

Invalid values throw `std::invalid_argument` during `pipeline` construction.

### Threshold behavior

- Writer pauses when buffered bytes reach `pause_writer_threshold`.
- Writer resumes when either:
- total buffered bytes drop to `resume_writer_threshold` or below
- unexamined bytes drop to `resume_writer_threshold` or below

Exact resume condition:

```cpp
writer_resumes =
    buffered_bytes <= resume_writer_threshold ||
    unexamined_bytes <= resume_writer_threshold;
```

This second rule is important for parser loops that examine data before consuming it.

Why this does not permit unbounded growth when the parser examines data but consumes nothing:

- If the reader repeatedly examines all currently buffered data, `unexamined_bytes` falls to 0 after each `advance(consumed, examined)` call.
- That satisfies the resume condition and allows the writer to make progress.
- The writer is still paused again whenever total buffered bytes reach `pause_writer_threshold`.
- As a result, growth remains bounded by the configured pause/resume thresholds and ongoing reader progress; repeatedly examining data does not disable backpressure.

Resume state table:

| buffered_bytes | unexamined_bytes | Result |
|---|---|---|
| above `resume_writer_threshold` | above `resume_writer_threshold` | writer remains paused |
| below-or-equal `resume_writer_threshold` | above `resume_writer_threshold` | writer resumes |
| above `resume_writer_threshold` | below-or-equal `resume_writer_threshold` | writer resumes |
| below-or-equal `resume_writer_threshold` | below-or-equal `resume_writer_threshold` | writer resumes |

Large writes are not atomic with respect to backpressure:

- `write()` appends bytes incrementally.
- If a write reaches the pause threshold partway through, that same `write()` call blocks mid-write.
- After the reader advances enough data to satisfy resume policy, the same `write()` call continues appending the remaining bytes.
- Backpressure is therefore applied during a large write, not only before or after it.

Example:

```cpp
pause_writer_threshold = 128;
writer.write(buffer_of_1024_bytes);
```

Actual behavior:

- The call may append some prefix of the 1024 bytes.
- Once buffered bytes reach the pause threshold, the call blocks.
- After the reader advances enough data, the same call resumes and writes more bytes.
- The function returns only after all 1024 bytes have been written or an error occurs.

Minimal example:

```cpp
xtd::pipeline pipe(xtd::pipe_options{
    .buffer_size = 64,
    .resume_writer_threshold = 64,
    .pause_writer_threshold = 128,
});

auto& writer = pipe.writer();
auto& reader = pipe.reader();
const std::string payload(1024, 'x');

auto producer = std::async(std::launch::async, [&]() {
    writer.write(payload); // blocks partway through
    writer.complete();
});

while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    // First read typically reaches the pause threshold (128 bytes here).
    reader.advance(seq.end(), seq.end());

    if (rr.completed()) break;
}

producer.wait();
reader.complete();
```

This behavior is covered by `tests/pipelines.cpp`.

## 4) `xtd::pipeline`

### Synopsis

```cpp
namespace xtd {

class pipeline {
public:
    explicit pipeline(pipe_options options = {});

    [[nodiscard]] 
    pipe_reader& reader() noexcept;

    [[nodiscard]] 
    pipe_writer& writer() noexcept;
};

}
```

### Notes

- `pipeline` owns shared state for one reader endpoint and one writer endpoint object.
- `reader()` and `writer()` return references to those endpoint objects.
- Endpoint objects themselves are non-copyable and non-movable.

## 5) `xtd::pipe_writer`

### Synopsis

```cpp
namespace xtd {

class pipe_writer {
public:
    std::size_t write(const std::byte* data, std::size_t length);

    template<class T>
    std::size_t write(const T& value);

    void complete() noexcept;
};

}
```

### `write(const std::byte*, std::size_t)`

Writes raw bytes.

- Throws `std::invalid_argument` if `data == nullptr && length > 0`.
- `write(nullptr, 0)` is a valid no-op.
- Throws `std::runtime_error` if writer is completed.
- Throws `std::runtime_error` if reader is completed.
- May block due to backpressure.
- A large write may block after partially appending its input; it is not atomic with respect to backpressure.
- Returns the number of bytes written (`length` on success).

### `write(const T&)`

Behavior by type category:

- If `T` is convertible to `std::string_view`: writes string bytes.
- Else if `T` is trivially copyable: writes `sizeof(T)` raw bytes.
- Otherwise: this overload does not participate in overload resolution.

Supported-type rule:

- This overload is available only for types that are convertible to `std::string_view` or trivially copyable.
- Unsupported types are rejected at compile time rather than producing a runtime exception.

Practical examples:

- `writer.write("abc")` writes 3 bytes.
- `writer.write(std::string("abc"))` writes 3 bytes.
- `writer.write(std::array<uint32_t, 3>{...})` writes raw array bytes.
- Non-trivially-copyable non-string-like structs must be serialized manually.

Portability warning:

- Writing a trivially copyable object writes its native object representation.
- This format is not inherently portable between architectures, processes built with different ABIs, or machines with different byte order.
- Raw object bytes may depend on endianness, integer representation, ABI/layout rules, and compiler/platform choices.
- Padding bytes inside structs may contain unspecified values and will also be written.
- For network or persistent formats, serialize fields explicitly and use a defined byte order.

### `complete()`

- Marks writer completed.
- Unblocks threads currently waiting in `read()` or `write()` on this pipeline.
- `pipe_writer::complete()` is idempotent.
- After `pipe_writer::complete()`, subsequent `write(...)` calls throw `std::runtime_error`.

## 6) `xtd::pipe_reader`

### Synopsis

```cpp
namespace xtd {

class pipe_reader {
public:
    read_result read();

    void advance(const position& consumed, const position& examined);
    void advance(const position& consumed);
    void advance(const segmented_byte_view& sequence);

    void complete() noexcept;
};

}
```

### `read()`

Returns a snapshot (`read_result`) of currently visible bytes.

- Blocks while no data is available and writer is not completed.
- Throws `std::runtime_error` if called again before `advance(...)`.
- Throws `std::runtime_error` if reader has been completed.

### `advance(consumed, examined)`

Reports progress for the most recent `read()` buffer.

- `consumed <= examined` is required.
- Positions must come from the most recent read buffer.
- Throws `std::runtime_error` if there is no pending read.
- Throws `std::runtime_error` if reader is completed.
- Throws `std::invalid_argument` for stale/foreign/out-of-range positions.

Behavioral meaning:

- `consumed`: bytes that can be discarded.
- `examined`: bytes that parser looked at.

### Convenience overloads

- `advance(consumed)` is shorthand for `advance(consumed, consumed)`.
- Semantic meaning: bytes up to `consumed` were consumed, and nothing beyond `consumed` was examined.
- `advance(sequence)` is shorthand for `advance(sequence.begin(), sequence.end())`.
- Semantic meaning: consume up to `sequence.begin()` and mark up to `sequence.end()` as examined.

### `complete()`

- Completes reader.
- Clears buffered state and pending read state.
- Subsequent `read()` and `advance(...)` calls throw `std::runtime_error`.
- Unblocks threads currently waiting in `write()`.
- `pipe_reader::complete()` is idempotent.
- Calling `pipe_reader::complete()` before or after `pipe_writer::complete()` is valid.

## 7) `xtd::read_result`

### Synopsis

```cpp
namespace xtd {

struct read_result {
    read_result(const read_result&) = delete;
    read_result& operator=(const read_result&) = delete;

    read_result(read_result&&) noexcept = default;
    read_result& operator=(read_result&&) noexcept = default;

    [[nodiscard]] 
    segmented_byte_view buffer() const noexcept;
    [[nodiscard]] 
    bool completed() const noexcept;
};

}
```

### Notes

- Move-only object.
- `completed()` indicates writer completion state at read time.
- `completed()` can be `true` while `buffer()` is non-empty.
- Copying the returned `segmented_byte_view` is typically cheap relative to copying payload bytes: it copies sequence metadata and segment/span views, not the underlying byte storage.
- `segmented_byte_view` is a non-owning view over pipeline-owned memory; copies of the sequence share the same underlying snapshot lifetime.

### Buffer lifetime

A buffer returned by `read_result::buffer()`, including all slices, positions, segment spans, and byte spans derived from it, remains valid only until the matching call to `pipe_reader::advance(...)`, `pipe_reader::complete()`, or destruction of the pipeline.

Do not retain pointers, spans, positions, or sequences after advancing the reader.

Invalid example:

```cpp
auto result = reader.read();
auto sequence = result.buffer();
auto segments = sequence.segments();

reader.advance(sequence.end());

// Invalid: segments refers to the previous read snapshot.
consume(segments);
```

## 8) `xtd::segmented_byte_view`

Represents a logical byte range that may span multiple segments.

### Key members

```cpp
const position& begin() const noexcept;
const position& end() const noexcept;
std::size_t size() const noexcept;
bool empty() const noexcept;

std::size_t segment_count() const;
std::span<const std::span<const std::byte>> segments() const noexcept;

segmented_byte_view slice(const position& end) const;
segmented_byte_view slice(const position& begin, const position& end) const;
segmented_byte_view slice(std::size_t beginOffset, const position& end) const;
segmented_byte_view slice(std::size_t beginOffset, std::size_t size) const;

position position_of(std::byte value) const;
position position_of(char value) const;

std::size_t copy_to(std::byte* destination, std::size_t destinationSize) const;
std::size_t copy_to(std::vector<std::byte>& destination) const;

template<class T>
bool copy_to(T& destination) const;

std::string to_string() const;
```

### Slicing semantics

- `slice(const position& end)` returns `[begin, end)`.
- `slice(const position& begin, const position& end)` returns `[begin, end)`.
- `slice(beginOffset, size)` uses offsets relative to current sequence begin.
- Position-based slicing requires positions from the same sequence identity.

### Copying semantics

- `copy_to(nullptr, n)` throws when `n > 0`.
- Copy count is truncated to `min(sequence.size(), destinationSize)`.
- `copy_to(T&)` requires trivially copyable `T` and exact size match.

### Multi-segment behavior

- `to_string()`, `copy_to(...)`, `position_of(...)`, and `slice(...)` work across segment boundaries.
- `position_of(...)` returns a `position`; when no match exists, it returns an invalid `position{}`.
- Prefer condition declarations such as `while (xtd::position pos = seq.position_of(...))`.

## 9) `xtd::position`

A lightweight cursor in a `segmented_byte_view`.

### Synopsis

```cpp
namespace xtd {

struct position {
    position() = default;

    explicit operator bool() const noexcept;

    position operator+(std::size_t offset) const noexcept;
    position& operator+=(std::size_t offset) noexcept;
    position& operator++() noexcept;
    position operator++(int) noexcept;

    bool operator==(const position& rhs) const noexcept;
};

}
```

### Notes

- Carries internal sequence identity.
- Supports condition-declaration patterns via explicit `operator bool()`.
- Use positions from the same read buffer when calling `advance()` or `slice(...)`.
- `operator+`, `operator+=`, and increment operators do not perform bounds checking against a sequence end.
- If position arithmetic produces an offset outside the valid range of the target sequence, later calls to `slice(...)` or `advance(...)` reject that position.
- Position arithmetic uses unsigned `std::size_t` math. Callers must avoid integer overflow; wrapped arithmetic does not represent a valid logical forward position.

### Arithmetic validity

Position arithmetic does not clamp to the end of a sequence.

```cpp
xtd::position past_end = seq.end() + 1;
```

- `past_end` keeps the same sequence identity, but its offset is outside the sequence range.
- Passing such a position to `slice(...)` or `advance(...)` throws because the position is out of range for that sequence/read buffer.
- The arithmetic operators themselves are `noexcept` and perform no bounds checks.

## 10) `xtd::pipe_utils`

### Synopsis

```cpp
namespace xtd {

struct pipe_utils {
    static std::thread threaded_copy_file_from_path(
        const std::string& path,
        pipe_writer& writer,
        std::size_t chunkSize = 4096);

    static std::thread threaded_copy_from_socket(
        int socketFd,
        pipe_writer& writer,
        std::size_t chunkSize = 4096);
};

}
```

### Behavior

- Starts a background thread and returns it.
- Worker thread always calls `writer.complete()` before exit.

### Errors

- `threaded_copy_file_from_path(...)`:
- throws `std::invalid_argument` for `chunkSize == 0`
- throws `std::runtime_error` when file cannot be opened initially
- `threaded_copy_from_socket(...)`:
- throws `std::invalid_argument` for `socketFd < 0`
- throws `std::invalid_argument` for `chunkSize == 0`

## 11) Semantics and Guarantees

### Read/advance protocol

For each successful `read()`:

1. Parse `read_result.buffer()`.
2. Call exactly one `advance(...)` for that read.
3. Call `read()` again.

Calling `read()` twice without `advance(...)` is an error.

### Completion semantics

- Writer completion does not drop buffered bytes.
- Reader can observe remaining bytes with `completed() == true`.
- After buffered bytes are drained, `read()` returns empty buffer with `completed() == true`.
- Reader completion clears pending read state, terminates further read/advance calls, and wakes blocked writers.
- Calling `pipe_writer::complete()` multiple times has no additional effect beyond the first completion.
- Calling `pipe_reader::complete()` multiple times has no additional effect beyond the first completion.
- After `pipe_writer::complete()`, later `write(...)` calls fail with `std::runtime_error`.
- `pipe_reader::complete(); pipe_writer::complete();` is valid; both endpoints remain completed.
- `completed() == true` does not imply the current buffer ends at a logical message boundary.
- Parser code must decide what to do with trailing incomplete data still present when completion is observed.

### Examined vs consumed interaction

- If you consume none and examine all, next `read()` may wait for data size change.
- If you consume none and examine none, next `read()` can return immediately with same data.
- Advancing examined can still help unblock writers under threshold policy.

### Backpressure

Writer pause/resume is threshold driven:

- pause when `buffered >= pause_writer_threshold`
- resume when total buffered or unexamined bytes fall to `resume_writer_threshold` or below

Backpressure can stop a single large `write()` call partway through.

- It does not wait for one whole `write()` call to finish before applying pause logic.
- It does not reject the write before writing anything unless the writer is already paused when the call begins.
- It does not guarantee that one `write()` may freely exceed the pause threshold and finish in one uninterrupted step.

### Thread-safety

A pipeline supports one logical writer and one logical reader.

- The writer side and reader side may be used concurrently from separate threads.
- `pipe_writer::write()` may run concurrently with `pipe_reader::read()`, `pipe_reader::advance(...)`, and `pipe_reader::complete()`.
- `pipe_writer::complete()` may run concurrently with `pipe_reader::read()`, `pipe_reader::advance(...)`, and `pipe_reader::complete()`.
- `pipe_writer::write()` and `pipe_writer::complete()` are internally synchronized with each other.
- `pipe_reader::read()`, `pipe_reader::advance(...)`, and `pipe_reader::complete()` are internally synchronized with each other.

Reader-side protocol remains single-consumer:

- Concurrent calls on the same `pipe_reader` are not a supported usage model.
- In particular, two threads should not call `read()` concurrently, and one thread should not call `advance(...)` while another thread is still processing the same pending read.

Writer-side notes:

- Concurrent calls on the same `pipe_writer` are serialized by internal synchronization.
- However, the pipeline is documented as one logical writer; do not treat it as a multi-writer message queue with guaranteed per-thread ordering or fairness.

Blocking behavior is synchronization-based; plan lifetime/order of `complete()` and thread joins accordingly.

## 12) Complexity

Typical complexity (not counting blocking wait time):

- `pipe_writer::write`: O(n) for `n` bytes written
- `pipe_reader::read`: O(s) where `s` is number of visible segments
- `segmented_byte_view::slice`: O(s) in multi-segment case, near O(1) in single-segment fast path
- `segmented_byte_view::position_of`: O(n)
- `segmented_byte_view::copy_to`: O(n) where n is `min(destination_size, origin_size)`

## 13) How-to Guides

### How to parse newline-delimited data

```cpp
xtd::pipeline pipe;
auto& writer = pipe.writer();
auto& reader = pipe.reader();

writer.write("one\ntwo\n");
writer.complete();

while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    while (xtd::position pos = seq.position_of('\n')) {
        std::string line = seq.slice(pos).to_string();
        // process line (line excludes '\n')
        seq = seq.slice(pos + 1, seq.end());
    }

    reader.advance(seq.begin(), seq.end());
    if (rr.completed()) {
        if (!seq.empty()) {
            // Trailing data without a final delimiter.
            // Choose a policy: process it, reject it, or discard it deliberately.
        }
        break;
    }
}
```

If the input may end with an incomplete trailing record such as `"one\ntwo"`, the final `seq` may contain `"two"` when `rr.completed()` becomes `true`.

### How to parse binary headers safely

```cpp
struct Header {
    std::uint32_t id;
    std::uint32_t payload_size;
};

while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    while (seq.size() >= sizeof(Header)) {
        Header h{};
        if (!seq.slice(0, sizeof(Header)).copy_to(h)) {
            break;
        }

        // parsed header
        seq = seq.slice(sizeof(Header), seq.end());
    }

    reader.advance(seq.begin(), seq.end());
    if (rr.completed()) {
        if (!seq.empty()) {
            throw std::runtime_error("Incomplete frame at end of stream");
        }
        break;
    }
}
```

### How to serialize non-trivial types

```cpp
struct Message {
    std::uint32_t id;
    std::string text;
};

void write_message(xtd::pipe_writer& writer, const Message& m) {
    writer.write(m.id);
    writer.write(static_cast<std::uint32_t>(m.text.size()));
    writer.write(m.text);
}
```

`writer.write(T)` does not accept non-trivial, non-string-like objects directly. Serialize field-by-field.

### How to keep partial data for incremental parsing

```cpp
while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    if (xtd::position pos = seq.position_of('\n')) {
        const xtd::position consumed = pos + 1; // consume through delimiter
        reader.advance(consumed, seq.end());
    } else {
        // No full message yet: keep all bytes, but mark all examined.
        // This can make the next read wait for new data/size change.
        reader.advance(seq.begin(), seq.end());
    }

    if (rr.completed()) break;
}
```

### How to stream from a file

```cpp
xtd::pipeline pipe;
std::thread producer = xtd::pipe_utils::threaded_copy_file_from_path("input.bin", pipe.writer());

auto& reader = pipe.reader();
std::size_t total = 0;

while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    total += seq.size();
    reader.advance(seq.end(), seq.end());
    // Equivalent convenience call: reader.advance(seq.end());
    // (consumed == examined == seq.end())

    if (rr.completed()) break;
}

reader.complete();
producer.join();
```

### How to stream from a socket

```cpp
xtd::pipeline pipe;
std::thread producer = xtd::pipe_utils::threaded_copy_from_socket(socketFd, pipe.writer(), 4096);

auto& reader = pipe.reader();
while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    reader.advance(seq.end(), seq.end());

    if (rr.completed()) break;
}

reader.complete();
producer.join();
```

### How to avoid unnecessary copies

```cpp
while (const xtd::read_result rr = reader.read()) {
    xtd::segmented_byte_view seq = rr.buffer();

    while (xtd::position delimiter = seq.position_of('|')) {
        // Parse using slices/positions first.
        xtd::segmented_byte_view field = seq.slice(delimiter);
        std::string text = field.to_string(); // materialize only when needed
        seq = seq.slice(delimiter + 1, seq.end());
    }

    reader.advance(seq.begin(), seq.end());
    if (rr.completed()) break;
}
```

## 14) Examples

### Example A: Minimal text pipeline

```cpp
#include "pipeline/pipeline.h"

int main() {
    xtd::pipeline pipe;
    auto& writer = pipe.writer();
    auto& reader = pipe.reader();

    writer.write("hello");
    writer.complete();

    std::string result;
    while (const xtd::read_result rr = reader.read()) {
        xtd::segmented_byte_view seq = rr.buffer();

        result += seq.to_string();
        reader.advance(seq.begin(), seq.end());

        if (rr.completed()) break;
    }

    if (result != "hello") return 1;
    reader.complete();
    return 0;
}
```

### Example B: Delimiter parser across segmented buffers

```cpp
#include <vector>
#include <string>
#include "pipeline/pipeline.h"

int main() {
    xtd::pipeline pipe(xtd::pipe_options{ .buffer_size = 3 });
    auto& writer = pipe.writer();
    auto& reader = pipe.reader();

    writer.write("ab\ncd\nef\n");
    writer.complete();

    std::vector<std::string> lines;

    while (const xtd::read_result rr = reader.read()) {
        xtd::segmented_byte_view seq = rr.buffer();

        while (xtd::position pos = seq.position_of('\n')) {
            lines.push_back(seq.slice(pos).to_string());
            seq = seq.slice(pos + 1, seq.end());
        }

        reader.advance(seq.begin(), seq.end());
        if (rr.completed()) {
            if (!seq.empty()) {
                // Final trailing bytes without '\n'.
                // Choose a policy: process them, reject them, or discard them deliberately.
            }
            break;
        }
    }

    return lines.size() == 3 ? 0 : 1;
}
```

### Example C: Backpressure with producer thread

```cpp
#include <future>
#include <thread>
#include "pipeline/pipeline.h"

int main() {
    using namespace std::chrono_literals;

    xtd::pipeline pipe(xtd::pipe_options{
        .buffer_size = 4,
        .resume_writer_threshold = 4,
        .pause_writer_threshold = 8,
    });

    auto& writer = pipe.writer();
    auto& reader = pipe.reader();

    auto producer = std::async(std::launch::async, [&]() {
        writer.write("12345678");
        writer.write("abcd"); // will block until reader advances
        writer.complete();
    });

    bool firstRead = true;
    std::string lastView;

    while (const xtd::read_result rr = reader.read()) {
        xtd::segmented_byte_view seq = rr.buffer();
        lastView = seq.to_string();

        if (firstRead) {
            reader.advance(seq.slice(0, 4).end());
            firstRead = false;
        } else {
            reader.advance(seq.end());
        }

        if (rr.completed()) break;
    }

    producer.wait();
    if (lastView != "5678abcd") return 1;
    reader.complete();

    return 0;
}
```

## See also

- `tests/pipelines.cpp` for behavior-oriented and edge-case coverage.
- `src/pipeline/pipeline.h`
- `src/pipeline/segmented_byte_view.h`
- `src/pipeline/pipe_utils.h`
