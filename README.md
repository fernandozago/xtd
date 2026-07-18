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
|               20.46 |       48,881,787.85 |    1.6% |      6.86 | `single-thread / bounded_channel`
|               22.09 |       45,276,617.79 |    0.6% |      6.82 | `single-thread / unbounded_channel`
|              158.56 |        6,306,778.01 |    1.4% |      6.82 | `multi-thread / bounded_channel`
|              170.80 |        5,854,849.28 |    0.9% |      6.89 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             335,450,143 | `single-thread / bounded_channel`
|             308,709,060 | `single-thread / unbounded_channel`
|              42,424,957 | `multi-thread / bounded_channel`
|              41,409,573 | `multi-thread / unbounded_channel`

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
|              224.03 |                4.46 |    1.0% |      5.39 | `1 KiB writes`
|              137.15 |                7.29 |    1.2% |      5.46 | `2 KiB writes`
|               90.86 |               11.01 |    1.8% |      5.41 | `4 KiB writes`
|               73.47 |               13.61 |    1.2% |      5.44 | `8 KiB writes`
|               69.61 |               14.37 |    1.9% |      5.58 | `16 KiB writes`
|               74.91 |               13.35 |    1.2% |      5.46 | `32 KiB writes`

|    Total Transfered | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           24.30 GiB | 1 KiB writes
|           40.15 GiB | 2 KiB writes
|           59.67 GiB | 4 KiB writes
|           74.89 GiB | 8 KiB writes
|           81.86 GiB | 16 KiB writes
|           72.61 GiB | 32 KiB writes
