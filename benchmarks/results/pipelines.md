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
|              269.77 |            3,706.89 |    1.2% |      6.89 | `1 KB writes`
|              166.38 |            6,010.27 |    0.7% |      6.88 | `2 KB writes`
|              116.18 |            8,607.60 |    0.8% |      6.90 | `4 KB writes`
|               82.87 |           12,067.25 |    0.8% |      6.89 | `8 KB writes`
|               79.89 |           12,517.02 |    2.1% |      6.95 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.11 GB | `1 KB writes`
|            40.72 GB | `2 KB writes`
|            58.64 GB | `4 KB writes`
|            81.77 GB | `8 KB writes`
|            87.95 GB | `16 KB writes`
