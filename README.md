# xtd
A header-only C++ library (namespace xtd) with two concurrency primitives:

- [Channel API reference](src/channel/channel.md)
- [Pipeline API reference](src/pipeline/pipeline.md)

## Benchmarks (Channel)

**Machine spec:**
```
CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
CPU cores (physical/logical per socket): 4 / 8
Memory: 14.4626 GiB total
```

> **Warning:** results might be unstable — CPU frequency scaling enabled (400–3700 MHz), turbo enabled.

### single-thread channel push/read

|          ns/message |           message/s |    err% |     total | benchmark |
|--------------------:|--------------------:|--------:|----------:|:----------|
|               16.94 |       59,029,795.04 |    0.7% |     11.39 | `(STACK OBJECT, INLINE STORAGE) bounded_channel / push / read` |
|               17.12 |       58,414,984.78 |    1.2% |     11.70 | `(HEAP OBJECT, INLINE STORAGE) bounded_channel / push / read` |
|               14.31 |       69,864,820.46 |    0.4% |     11.50 | `(STACK OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read` |
|               14.51 |       68,940,101.85 |    0.4% |     11.63 | `(HEAP OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read` |

## Benchmarks (Pipeline)

**Soon**