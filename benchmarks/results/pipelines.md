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
|              269.65 |            3,708.50 |    2.8% |      6.78 | `1 KB writes`
|              164.51 |            6,078.72 |    1.5% |      6.76 | `2 KB writes`
|              120.98 |            8,265.78 |    2.6% |      6.84 | `4 KB writes`
|               86.40 |           11,573.98 |    2.0% |      6.93 | `8 KB writes`
|               81.23 |           12,310.25 |    1.5% |      6.79 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.51 GB | `1 KB writes`
|            40.71 GB | `2 KB writes`
|            55.32 GB | `4 KB writes`
|            78.88 GB | `8 KB writes`
|            82.61 GB | `16 KB writes`
