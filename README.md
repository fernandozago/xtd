# xtd
A header-only C++ library (namespace xtd) with two concurrency primitives:

- [Channel API reference](src/channel/channel.md)
- [Pipeline API reference](src/pipeline/pipeline.md)

## Channel Benchmark

**Machine spec:**
```
CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
CPU cores (physical/logical per socket): 4 / 8
Memory: 14.4626 GiB total
```

> **Warning:** results might be unstable — CPU frequency scaling enabled (400–3700 MHz), turbo enabled.

### single-thread channel push/read

|          ns/message |           message/s |    err% |     total | single-thread channel push/read
|--------------------:|--------------------:|--------:|----------:|:--------------------------------
|               21.87 |       45,721,955.35 |    0.2% |     46.01 | `(STACK OBJECT, INLINE STORAGE) bounded_channel / push / read`
| | | | | **Total messages transferred: 2125216954**
|               23.52 |       42,510,080.04 |    1.7% |     47.15 | `(HEAP OBJECT, INLINE STORAGE) bounded_channel / push / read`
| | | | | **Total messages transferred: 1974171712**
|               20.69 |       48,330,730.84 |    1.6% |     45.82 | `(STACK OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read`
| | | | | **Total messages transferred: 2214022708**
|               20.45 |       48,888,865.87 |    0.3% |     46.98 | `(HEAP OBJECT, HEAP-BACKED DEQUE) unbounded_channel / push / read`
| | | | | **Total messages transferred: 2277796794**

## Pipeline Benchmark

**Machine spec:**
```
CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
CPU cores (physical/logical per socket): 4 / 8
Memory: 14.4626 GiB total
```

> **Warning:** results might be unstable — CPU frequency scaling enabled (400–3700 MHz), turbo enabled.
### multi-thread pipeline (one thread for reading / one thread for writing)

|              ms/GiB |               GiB/s |    err% |     total | xtd::pipeline writer throughput
|--------------------:|--------------------:|--------:|----------:|:--------------------------------
|              244.15 |                4.10 |    0.9% |     13.29 | `1 KiB writes`
| | | | | **Total Bytes Transferred: 55.56 GiB**
|              152.20 |                6.57 |    0.9% |     13.23 | `2 KiB writes`
| | | | | **Total Bytes Transferred: 88.71 GiB**
|              103.34 |                9.68 |    0.7% |     12.90 | `4 KiB writes`
| | | | | **Total Bytes Transferred: 128.72 GiB**
|               87.06 |               11.49 |    0.5% |     13.05 | `8 KiB writes`
| | | | | **Total Bytes Transferred: 157.13 GiB**
|               79.21 |               12.62 |    0.8% |     13.10 | `16 KiB writes`
| | | | | **Total Bytes Transferred: 182.41 GiB**