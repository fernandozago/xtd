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
|              280.68 |            3,562.83 |    2.3% |      6.77 | `1 KB writes`
|              175.81 |            5,687.97 |    0.7% |      6.90 | `2 KB writes`
|              119.24 |            8,386.23 |    1.0% |      6.86 | `4 KB writes`
|               90.60 |           11,036.95 |    2.0% |      7.02 | `8 KB writes`
|               82.11 |           12,178.37 |    1.5% |      6.84 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.56 GB | `1 KB writes`
|            38.47 GB | `2 KB writes`
|            55.95 GB | `4 KB writes`
|            76.89 GB | `8 KB writes`
|            82.60 GB | `16 KB writes`
