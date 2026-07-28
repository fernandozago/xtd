Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|              μs/MB |                MB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              267.36 |            3,740.28 |    0.6% |      6.83 | `1 KB writes`
|              159.90 |            6,253.82 |    1.4% |      6.83 | `2 KB writes`
|              115.14 |            8,684.78 |    0.4% |      6.88 | `4 KB writes`
|               83.86 |           11,925.34 |    1.4% |      6.76 | `8 KB writes`
|               80.22 |           12,465.14 |    2.5% |      6.80 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.07 GB | `1 KB writes`
|            41.98 GB | `2 KB writes`
|            58.74 GB | `4 KB writes`
|            79.23 GB | `8 KB writes`
|            84.72 GB | `16 KB writes`
