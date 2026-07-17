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
|               21.05 |       47,502,094.54 |    0.8% |          274.00 |           29.93 |  9.153 |          69.00 |    0.0% |      5.43 | `single-thread / bounded_channel`
|               22.54 |       44,374,418.88 |    1.5% |          244.27 |           33.10 |  7.380 |          63.24 |    0.0% |      5.42 | `single-thread / unbounded_channel`
|              188.44 |        5,306,726.82 |    1.6% |          162.21 |          117.31 |  1.383 |          38.26 |    0.7% |      5.43 | `multi-thread / bounded_channel`
|              196.48 |        5,089,556.83 |    1.3% |          152.91 |          136.45 |  1.121 |          38.77 |    0.9% |      5.40 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             258,293,012 | `single-thread / bounded_channel`
|             240,916,971 | `single-thread / unbounded_channel`
|              29,138,695 | `multi-thread / bounded_channel`
|              27,703,363 | `multi-thread / unbounded_channel`

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
|              251.39 |                3.98 |    2.0% |  481,738,554.94 |  212,456,845.71 |  2.267 |  99,771,624.29 |    0.4% |      5.40 | `1 KiB writes`
|              157.01 |                6.37 |    0.7% |  299,423,632.33 |  128,271,770.22 |  2.334 |  55,916,969.71 |    0.6% |      5.48 | `2 KiB writes`
|              114.56 |                8.73 |    0.8% |  108,608,153.52 |   85,069,806.27 |  1.277 |  25,819,014.96 |    0.8% |      5.49 | `4 KiB writes`
|               92.09 |               10.86 |    1.0% |   70,782,699.66 |   64,737,855.77 |  1.093 |  16,554,093.24 |    0.8% |      5.47 | `8 KiB writes`
|               95.99 |               10.42 |    2.5% |   54,514,972.59 |   54,315,953.34 |  1.004 |  12,490,988.07 |    0.8% |      5.60 | `16 KiB writes`
|              105.17 |                9.51 |    1.9% |   47,289,807.54 |   57,188,843.99 |  0.827 |  10,692,849.72 |    0.8% |      5.55 | `32 KiB writes`

|    Total Transfered | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           21.67 GiB | 1 KiB writes
|           35.04 GiB | 2 KiB writes
|           48.18 GiB | 4 KiB writes
|           60.09 GiB | 8 KiB writes
|           59.81 GiB | 16 KiB writes
|           52.96 GiB | 32 KiB writes
