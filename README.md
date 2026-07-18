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

### channel throughput

|          ns/message |           message/s |    err% |     total | xtd::channel throughput
|--------------------:|--------------------:|--------:|----------:|:------------------------
|               19.83 |       50,430,054.70 |    0.6% |      6.75 | `single-thread / bounded_channel`
|               21.45 |       46,619,007.63 |    0.5% |      6.79 | `single-thread / unbounded_channel`
|              168.01 |        5,952,106.04 |    2.7% |      6.83 | `multi-thread / bounded_channel`
|              170.18 |        5,876,094.45 |    0.9% |      6.88 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             337,936,250 | `single-thread / bounded_channel`
|             316,023,492 | `single-thread / unbounded_channel`
|              42,081,642 | `multi-thread / bounded_channel`
|              41,793,590 | `multi-thread / unbounded_channel`

## Pipeline Benchmark

**Machine spec:**
```
CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
CPU cores (physical/logical per socket): 4 / 8
Memory: 14.4626 GiB total
```

> **Warning:** results might be unstable — CPU frequency scaling enabled (400–3700 MHz), turbo enabled.

### multi-thread pipeline (one thread for reading / one thread for writing)

|              ms/GiB |               GiB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              220.94 |                4.53 |    1.0% |      5.45 | `1 KiB writes`
|              136.16 |                7.34 |    0.9% |      5.48 | `2 KiB writes`
|               91.58 |               10.92 |    0.9% |      5.55 | `4 KiB writes`
|               73.72 |               13.57 |    1.5% |      5.51 | `8 KiB writes`
|               68.33 |               14.63 |    1.5% |      5.51 | `16 KiB writes`
|               76.77 |               13.03 |    1.7% |      5.49 | `32 KiB writes`

|    Total Transfered | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           24.72 GiB | 1 KiB writes
|           40.37 GiB | 2 KiB writes
|           60.93 GiB | 4 KiB writes
|           75.19 GiB | 8 KiB writes
|           82.58 GiB | 16 KiB writes
|           71.56 GiB | 32 KiB writes
