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

|          ns/message |           message/s |    err% |     ins/message |     cyc/message |    IPC |    bra/message |   miss% |     total | xtd::channel throughput
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:------------------------
|               21.50 |       46,517,007.24 |    0.5% |          274.00 |           31.21 |  8.778 |          69.00 |    0.0% |      6.81 | `single-thread / bounded_channel`
|               22.29 |       44,854,739.98 |    0.5% |          244.27 |           33.02 |  7.398 |          63.24 |    0.0% |      6.69 | `single-thread / unbounded_channel`
|              184.57 |        5,417,887.71 |    1.3% |          161.49 |          116.83 |  1.382 |          38.16 |    0.7% |      6.78 | `multi-thread / bounded_channel`
|              193.04 |        5,180,241.20 |    1.5% |          152.35 |          134.30 |  1.134 |          38.62 |    0.8% |      6.70 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             315,585,333 | `single-thread / bounded_channel`
|             296,084,277 | `single-thread / unbounded_channel`
|              36,553,875 | `multi-thread / bounded_channel`
|              34,839,619 | `multi-thread / unbounded_channel`

## Pipeline Benchmark

**Machine spec:**
```
CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
CPU cores (physical/logical per socket): 4 / 8
Memory: 14.4626 GiB total
```

> **Warning:** results might be unstable — CPU frequency scaling enabled (400–3700 MHz), turbo enabled.

### multi-thread pipeline (one thread for reading / one thread for writing)

|              ms/GiB |               GiB/s |    err% |         ins/GiB |         cyc/GiB |    IPC |        bra/GiB |   miss% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:-------------------------
|              229.78 |                4.35 |    2.8% |  463,068,106.17 |  194,411,980.29 |  2.382 |  97,989,865.88 |    0.5% |      5.43 | `1 KiB writes`
|              137.41 |                7.28 |    2.0% |  289,222,486.13 |  116,446,398.63 |  2.484 |  54,466,933.34 |    0.4% |      5.45 | `2 KiB writes`
|               98.17 |               10.19 |    1.4% |  102,913,669.15 |   76,839,013.68 |  1.339 |  24,729,815.13 |    0.7% |      5.49 | `4 KiB writes`
|               79.76 |               12.54 |    1.9% |   70,372,179.72 |   59,717,158.17 |  1.178 |  16,410,991.26 |    0.7% |      5.52 | `8 KiB writes`
|               83.26 |               12.01 |    0.9% |   53,547,978.24 |   48,841,508.93 |  1.096 |  12,056,178.37 |    0.7% |      5.49 | `16 KiB writes`
|               94.96 |               10.53 |    1.1% |   48,861,581.21 |   52,895,490.05 |  0.924 |  10,856,557.36 |    0.8% |      5.48 | `32 KiB writes`

|    Total Transfered | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           23.85 GiB | 1 KiB writes
|           39.66 GiB | 2 KiB writes
|           56.76 GiB | 4 KiB writes
|           69.92 GiB | 8 KiB writes
|           67.41 GiB | 16 KiB writes
|           57.37 GiB | 32 KiB writes
