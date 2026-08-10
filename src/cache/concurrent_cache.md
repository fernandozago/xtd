# xtd Concurrent Cache Reference

This page documents the public concurrent cache API in a style similar to cppreference.

## Contents

1. [Overview](#1-overview)
2. [Header and public types](#2-header-and-public-types)
3. [`xtd::cache_entry_opts`](#3-xtdcache_entry_opts)
4. [`xtd::cache_value<T>`](#4-xtdcache_valuet)
5. [`xtd::concurrent_cache<key_t, value_t, ...>`](#5-xtdconcurrent_cachekey_t-value_t-)
6. [TTL and expiration](#6-ttl-and-expiration)
7. [Stampede protection](#7-stampede-protection)
8. [Sharding model](#8-sharding-model)
9. [Semantics and guarantees](#9-semantics-and-guarantees)
10. [Complexity](#10-complexity)
11. [How-to guides](#11-how-to-guides)
12. [Examples](#12-examples)

## 1) Overview

`xtd::concurrent_cache<key_t, value_t>` is a thread-safe key/value store for
shared, computed, or loaded values that benefit from caching.

Core model:

- values are stored as `std::shared_ptr<const value_t>` (`cache_value<T>`);
- concurrent readers share ownership through `shared_ptr` without copying the value;
- writes use an exclusive lock scoped to a single shard, keeping contention low;
- reads only require a shared lock on the relevant shard;
- optional per-entry TTL controls expiration via the configured clock;
- `get_or_create` prevents multiple threads from computing the same value at once
  (stampede protection).

Primary use cases:

- caching expensive or remotely loaded results shared across threads;
- lazy population of a shared lookup table;
- rate-limiting redundant computation with optional automatic expiration;
- thread-safe in-memory object pools keyed by identifier.

## 2) Header and public types

### Namespace

```cpp
namespace xtd {
    // ...
}
```

### Header

```cpp
#include "cache/concurrent_cache.h"
```

### Public types

```cpp
xtd::cache_value<T>       // alias for std::shared_ptr<const T>
xtd::cache_entry_opts     // per-entry TTL configuration
xtd::concurrent_cache<key_t, value_t, hash_t, key_equal_t, clock_t>
```

`concurrent_cache` is non-copyable and non-movable.

Values are always stored and returned as `cache_value<T>`, which is
`std::shared_ptr<const T>`. This makes it safe to hold a cached pointer after
the entry is evicted or replaced: the underlying object lives until all callers
release their `shared_ptr`.

## 3) `xtd::cache_entry_opts`

### Synopsis

```cpp
namespace xtd {

struct cache_entry_opts final {
    static constexpr std::chrono::nanoseconds max_supported_ttl = /* ~292 years */;

    cache_entry_opts();
    cache_entry_opts(std::chrono::nanoseconds ttl);

    std::chrono::nanoseconds m_ttl;
};

}
```

### Members

| Member | Meaning |
|---|---|
| `m_ttl` | Time-to-live for the entry. Zero means no expiration. |
| `max_supported_ttl` | Maximum allowed TTL (~292 years) |

### Default construction

```cpp
xtd::cache_entry_opts opts;
// opts.m_ttl == std::chrono::nanoseconds::zero()  →  no expiration
```

### TTL construction

```cpp
using namespace std::chrono_literals;
xtd::cache_entry_opts opts{5min};
```

### Assertions

The constructor asserts:

- `ttl >= nanoseconds::zero()`;
- `ttl <= cache_entry_opts::max_supported_ttl`.

Both conditions are checked at runtime via `assert`.

## 4) `xtd::cache_value<T>`

```cpp
template<typename value_t>
using cache_value = std::shared_ptr<const value_t>;
```

All cache operations return a `cache_value<T>`.

A null `cache_value` (i.e. `nullptr`) indicates a cache miss:

```cpp
if (const auto val = cache.get(key)) {
    use(*val);
} else {
    // key not found or expired
}
```

The caller may keep the `cache_value` alive past the lifetime of the cache entry:

```cpp
const auto val = cache.get(key);
cache.erase(key);   // removes cache's reference
// val still valid — the object lives while val is in scope
```

## 5) `xtd::concurrent_cache<key_t, value_t, ...>`

### Synopsis

```cpp
namespace xtd {

template<
    std::copy_constructible key_t,
    typename value_t,
    typename hash_t      = std::hash<key_t>,
    typename key_equal_t = std::equal_to<key_t>,
    typename clock_t     = std::chrono::steady_clock>
class concurrent_cache final {
public:
    explicit concurrent_cache(
        std::size_t shard_count = 16,
        hash_t      hash        = {},
        key_equal_t key_equal   = {});

    concurrent_cache(const concurrent_cache&) = delete;
    concurrent_cache& operator=(const concurrent_cache&) = delete;
    concurrent_cache(concurrent_cache&&) = delete;
    concurrent_cache& operator=(concurrent_cache&&) = delete;

    [[nodiscard]] cache_value<value_t> get(const key_t& key) const;
    [[nodiscard]] bool                 contains(const key_t& key) const;

    template<typename... Args>
    [[nodiscard]] cache_value<value_t> insert_or_assign(key_t key, cache_entry_opts options, Args&&... args);

    template<typename... Args>
    [[nodiscard]] cache_value<value_t> insert_or_assign(key_t key, Args&&... args);

    template<typename factory_t>
    [[nodiscard]] cache_value<value_t> get_or_create(const key_t& key, cache_entry_opts options, factory_t&& factory);

    template<typename factory_t>
    [[nodiscard]] cache_value<value_t> get_or_create(const key_t& key, factory_t&& factory);

    [[nodiscard]] bool        erase(const key_t& key);
    [[nodiscard]] std::size_t purge_expired() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] bool        empty() const;
};

}
```

### Template parameters

| Parameter | Constraint | Default |
|---|---|---|
| `key_t` | `std::copy_constructible` | — |
| `value_t` | Any type | — |
| `hash_t` | Callable `(key_t) → std::size_t` | `std::hash<key_t>` |
| `key_equal_t` | Callable `(key_t, key_t) → bool` | `std::equal_to<key_t>` |
| `clock_t` | `std::chrono::steady_clock`-compatible | `std::chrono::steady_clock` |

The `clock_t` parameter is primarily intended for testing with a controllable
manual clock. Production code should use the default `std::chrono::steady_clock`.

### Constructor

```cpp
explicit concurrent_cache(
    std::size_t shard_count = 16,
    hash_t      hash        = {},
    key_equal_t key_equal   = {});
```

Creates a cache with the specified number of independent shards.

`shard_count` must be greater than zero (enforced by `assert`).

Higher shard counts reduce lock contention under concurrent access at the cost
of additional memory for the shard structures. The default of 16 is suitable
for most workloads.

```cpp
xtd::concurrent_cache<int, std::string> cache;           // 16 shards
xtd::concurrent_cache<int, std::string> cache(64);       // 64 shards
```

---

### `get`

```cpp
[[nodiscard]]
cache_value<value_t> get(const key_t& key) const;
```

Returns the cached value for `key`, or `nullptr` on a miss or expired entry.

- Acquires a **shared lock** on the relevant shard (fast path).
- If the entry has expired, upgrades to an exclusive lock, removes it, and
  returns `nullptr`.
- The returned `cache_value` remains valid until all callers release it,
  regardless of subsequent cache mutations.

```cpp
if (const auto val = cache.get(key)) {
    process(*val);
}
```

---

### `contains`

```cpp
[[nodiscard]]
bool contains(const key_t& key) const;
```

Returns `true` if the key exists and its entry has not expired.

Acquires a shared lock. The result may be stale immediately after return due
to concurrent mutation. Do not use `contains` as a synchronization gate before
`get`; prefer `get` directly.

---

### `insert_or_assign`

```cpp
template<typename... Args>
[[nodiscard]]
cache_value<value_t> insert_or_assign(key_t key, cache_entry_opts options, Args&&... args);

template<typename... Args>
[[nodiscard]]
cache_value<value_t> insert_or_assign(key_t key, Args&&... args);
```

Constructs a new `value_t(args...)` and stores it under `key`.

- If `key` already exists, the previous entry is replaced.
- The old entry's destructor is called **outside** the shard lock to avoid
  holding the lock during potentially expensive cleanup.
- Returns a `cache_value` to the newly inserted value.
- The overload without `options` stores the entry without expiration.

```cpp
// Store with no expiration.
const auto val = cache.insert_or_assign(42, 100, "hello");

// Store with a 30-second TTL.
using namespace std::chrono_literals;
const auto val = cache.insert_or_assign(42, xtd::cache_entry_opts{30s}, 100, "hello");
```

---

### `get_or_create`

```cpp
template<typename factory_t>
[[nodiscard]]
cache_value<value_t> get_or_create(const key_t& key, cache_entry_opts options, factory_t&& factory);

template<typename factory_t>
[[nodiscard]]
cache_value<value_t> get_or_create(const key_t& key, factory_t&& factory);
```

Returns the cached value for `key` if it exists and is not expired. Otherwise,
invokes `factory(key)` to produce the value, stores it, and returns it.

The factory signature must be:

```cpp
value_t factory(const key_t& key);
```

**Stampede protection** is built in: if multiple threads call `get_or_create`
for the same key simultaneously, only one thread invokes the factory. The
others wait and receive the same result without invoking the factory themselves.
See [Stampede protection](#7-stampede-protection) for details.

```cpp
const auto val = cache.get_or_create(key, [](const int k) {
    return compute_expensive_value(k);
});
```

If the factory throws, the exception propagates to all threads waiting on that
key, and the entry is not stored.

---

### `erase`

```cpp
[[nodiscard]]
bool erase(const key_t& key);
```

Removes the entry for `key` from the cache.

Returns `true` if an entry was present and removed, `false` if the key was not found.

Callers holding a `cache_value` returned by a prior `get` or `insert_or_assign`
continue to own their `shared_ptr` and can safely access the value after erasure.

---

### `purge_expired`

```cpp
[[nodiscard]]
std::size_t purge_expired() const;
```

Scans all shards and removes all entries whose TTL has elapsed.

Returns the number of entries removed.

Expiration is lazy by default: entries are only removed on access or when
`purge_expired` is called. Call this periodically if memory reclamation
of stale entries is important.

```cpp
const std::size_t removed = cache.purge_expired();
```

---

### `size`

```cpp
[[nodiscard]]
std::size_t size() const;
```

Returns the number of non-expired entries across all shards at the time of
the call. This is a linear scan across all shards and acquires a shared lock
on each.

The result may be stale immediately after return.

---

### `empty`

```cpp
[[nodiscard]]
bool empty() const;
```

Returns `true` if there are no non-expired entries.

Short-circuits on the first live entry found; can be faster than `size() == 0`
when the cache is known to be non-empty.

## 6) TTL and expiration

Each entry may carry an optional time-to-live set via `cache_entry_opts`.

```cpp
using namespace std::chrono_literals;
xtd::cache_entry_opts short_lived{500ms};
xtd::cache_entry_opts long_lived{1h};
xtd::cache_entry_opts permanent{};   // zero TTL = never expires
```

An entry expires when `clock_t::now() >= insert_time + ttl`.

Expiration is checked at access time:

- `get` returns `nullptr` for an expired entry and removes it.
- `contains` returns `false` for an expired entry.
- `get_or_create` treats an expired entry as a miss and re-invokes the factory.
- `size` and `empty` exclude expired entries from their results.

Expired entries are not automatically removed from storage. Use `purge_expired`
to reclaim memory held by stale entries that have not been accessed.

### Custom clock

The `clock_t` template parameter allows substituting the clock, which is useful
for deterministic tests:

```cpp
struct manual_clock {
    using duration   = std::chrono::milliseconds;
    using time_point = std::chrono::time_point<manual_clock, duration>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept { return current_time; }
    inline static time_point current_time{};
};

xtd::concurrent_cache<int, int,
    std::hash<int>, std::equal_to<int>, manual_clock> cache;
```

## 7) Stampede protection

Without protection, a cache miss for a popular key under high concurrency can
cause many threads to simultaneously execute the same expensive factory,
wasting resources and potentially overloading downstream systems.

`get_or_create` prevents this:

1. The first thread that observes a miss registers an in-flight `std::shared_future`
   for the key before releasing the exclusive lock.
2. Subsequent threads for the same key find the in-flight entry and wait on
   the same future, releasing the lock immediately.
3. The owning thread runs the factory, stores the result, fulfills the promise,
   and clears the in-flight entry.
4. All waiting threads receive the same `cache_value` without executing the factory.

If the factory throws, the exception is propagated to all waiters through
`std::promise::set_exception`. No entry is stored.

```cpp
// Only one thread will execute the lambda, regardless of concurrency.
const auto val = cache.get_or_create(key, [](const std::string& k) {
    return fetch_from_database(k);     // expensive — called exactly once per miss
});
```

This guarantee applies per key per miss event. If the entry expires and is
missed again, the factory will be invoked once more.

## 8) Sharding model

The cache distributes entries across `shard_count` independent shards. Each
shard has its own `std::shared_mutex`, its own value map, and its own in-flight
map.

A key is assigned to a shard by:

```
shard_index = hash(key) % shard_count
```

Operations on different shards never contend. Operations on the same shard
use the following locking strategy:

| Operation | Lock type |
|---|---|
| `get` (hit, no expiry) | Shared |
| `get` (expired entry removal) | Upgrade to exclusive |
| `contains` | Shared |
| `insert_or_assign` | Exclusive |
| `get_or_create` fast path (hit) | Shared |
| `get_or_create` miss registration | Exclusive |
| `get_or_create` result publication | Exclusive |
| `erase` | Exclusive |
| `purge_expired` per shard | Exclusive |
| `size` per shard | Shared |
| `empty` per shard | Shared |

Destructors of evicted or replaced entries are always called **outside** any
shard lock to avoid holding the mutex during potentially costly cleanup.

## 9) Semantics and guarantees

### Thread safety

All public methods are safe to call concurrently from multiple threads.

Multiple threads may call `get`, `contains`, `insert_or_assign`, `get_or_create`,
`erase`, and `purge_expired` simultaneously without external synchronization.

### Value immutability

Stored values are `const`. A `cache_value<T>` is `std::shared_ptr<const T>`.
Callers cannot mutate a cached value in place; they must store a replacement
via `insert_or_assign`.

### Returned pointer stability

A `cache_value` returned by any operation remains valid for the caller's
lifetime of that `shared_ptr`, regardless of subsequent insertions, erasures,
or replacements affecting the same key.

### Expiration is not eviction

An expired entry continues to occupy memory until one of:

- `get` observes it and removes it;
- `get_or_create` observes it and replaces it;
- `insert_or_assign` replaces it;
- `erase` removes it explicitly;
- `purge_expired` scans and removes it.

### Non-copyable, non-movable

```cpp
concurrent_cache(const concurrent_cache&) = delete;
concurrent_cache& operator=(const concurrent_cache&) = delete;
concurrent_cache(concurrent_cache&&) = delete;
concurrent_cache& operator=(concurrent_cache&&) = delete;
```

Keep the cache at a stable address. Shared references should be passed as
raw references or wrapped in `std::shared_ptr<concurrent_cache<...>>`.

## 10) Complexity

Typical complexity, excluding lock wait time and allocation details:

| Operation | Typical complexity |
|---|---:|
| `get` (hit) | O(1) |
| `get` (miss or expired) | O(1) |
| `contains` | O(1) |
| `insert_or_assign` | O(1), plus construction of `value_t` |
| `get_or_create` (hit) | O(1) |
| `get_or_create` (miss, factory) | O(1) + factory cost |
| `erase` | O(1) |
| `purge_expired` | O(n) across all entries |
| `size` | O(n) across all entries |
| `empty` | O(1) amortized (short-circuits) |

All O(1) operations are amortized per the `std::unordered_map` guarantee.

## 11) How-to guides

### Store a value with no expiration

```cpp
xtd::concurrent_cache<int, std::string> cache;
cache.insert_or_assign(1, "hello");
```

### Store a value with a TTL

```cpp
using namespace std::chrono_literals;
cache.insert_or_assign(1, xtd::cache_entry_opts{10s}, "hello");
```

### Retrieve a value

```cpp
if (const auto val = cache.get(1)) {
    std::cout << *val << '\n';
}
```

### Lazy-populate with stampede protection

```cpp
const auto val = cache.get_or_create(key, [](const int k) {
    return load_from_db(k);
});
```

### Lazy-populate with TTL

```cpp
using namespace std::chrono_literals;
const auto val = cache.get_or_create(key, xtd::cache_entry_opts{1min}, [](const int k) {
    return load_from_db(k);
});
```

### Erase an entry

```cpp
const bool removed = cache.erase(key);
```

### Reclaim memory from expired entries

```cpp
const std::size_t count = cache.purge_expired();
```

### Tune shard count for high concurrency

```cpp
// 64 shards reduces per-shard contention for a heavily concurrent workload.
xtd::concurrent_cache<std::string, MyValue> cache(64);
```

### Use a custom hash and equality

```cpp
struct CaseInsensitiveHash { /* ... */ };
struct CaseInsensitiveEqual { /* ... */ };

xtd::concurrent_cache<
    std::string, MyValue,
    CaseInsensitiveHash,
    CaseInsensitiveEqual> cache;
```

## 12) Examples

### Basic insert and retrieve

```cpp
#include "cache/concurrent_cache.h"

int main()
{
    xtd::concurrent_cache<int, std::string> cache;

    cache.insert_or_assign(1, "one");
    cache.insert_or_assign(2, "two");

    if (const auto val = cache.get(1)) {
        // *val == "one"
    }

    return 0;
}
```

### Lazy loading with `get_or_create`

```cpp
#include "cache/concurrent_cache.h"

std::string load_from_disk(const std::string& path);

int main()
{
    xtd::concurrent_cache<std::string, std::string> cache;

    const auto content = cache.get_or_create("/etc/hostname", [](const std::string& path) {
        return load_from_disk(path);
    });

    // load_from_disk was called once; subsequent calls return the cached value.
    const auto again = cache.get_or_create("/etc/hostname", [](const std::string& path) {
        return load_from_disk(path);
    });

    return 0;
}
```

### Expiring entries

```cpp
#include "cache/concurrent_cache.h"

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    xtd::concurrent_cache<int, int> cache;

    cache.insert_or_assign(1, xtd::cache_entry_opts{100ms}, 42);

    // Returns 42 within the TTL window.
    assert(cache.get(1) != nullptr);

    std::this_thread::sleep_for(200ms);

    // Returns nullptr after expiration.
    assert(cache.get(1) == nullptr);

    return 0;
}
```

### Multi-threaded shared lookup

```cpp
#include "cache/concurrent_cache.h"

#include <thread>
#include <vector>

int expensive_compute(int key);

int main()
{
    xtd::concurrent_cache<int, int> cache(32);

    constexpr int num_threads = 8;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cache, i] {
            // All threads computing the same key will converge on one factory call.
            const auto val = cache.get_or_create(i % 4, [](const int k) {
                return expensive_compute(k);
            });
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

### Caller retains value after eviction

```cpp
#include "cache/concurrent_cache.h"

int main()
{
    xtd::concurrent_cache<int, std::string> cache;

    const xtd::cache_value<std::string> val = cache.insert_or_assign(1, "still alive");

    cache.erase(1);   // removes cache's reference

    // val is still valid here — the string lives until val goes out of scope.
    assert(*val == "still alive");

    return 0;
}
```

## See also

- [`tests/concurrent_cache.cpp`](../../tests/concurrent_cache.cpp)
- [`tests/caches/concurrent_cache.h`](../../tests/caches/concurrent_cache.h)
- [`tests/caches/concurrent_cache_expirations.h`](../../tests/caches/concurrent_cache_expirations.h)
- [`concurrent_cache.h`](concurrent_cache.h)
