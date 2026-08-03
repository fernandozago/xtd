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
|              308.79 |            3,238.49 |    1.5% |      6.86 | `1 KB writes`
|              187.04 |            5,346.38 |    1.4% |      6.93 | `2 KB writes`
|              122.87 |            8,138.54 |    1.2% |      6.74 | `4 KB writes`
|               90.06 |           11,104.23 |    1.8% |      6.85 | `8 KB writes`
|               82.67 |           12,096.22 |    0.6% |      6.90 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            21.79 GB | `1 KB writes`
|            36.23 GB | `2 KB writes`
|            53.90 GB | `4 KB writes`
|            75.43 GB | `8 KB writes`
|            82.56 GB | `16 KB writes`
