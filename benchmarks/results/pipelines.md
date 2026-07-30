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
|              271.15 |            3,688.00 |    2.0% |      7.07 | `1 KB writes`
|              156.51 |            6,389.55 |    3.1% |      6.70 | `2 KB writes`
|              107.57 |            9,296.08 |    0.8% |      6.87 | `4 KB writes`
|               82.16 |           12,171.71 |    1.2% |      6.88 | `8 KB writes`
|               80.06 |           12,490.95 |    1.2% |      6.83 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.86 GB | `1 KB writes`
|            42.23 GB | `2 KB writes`
|            62.82 GB | `4 KB writes`
|            82.21 GB | `8 KB writes`
|            84.55 GB | `16 KB writes`
