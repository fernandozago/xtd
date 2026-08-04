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
|              276.96 |            3,610.64 |    1.1% |      6.91 | `1 KB writes`
|              171.18 |            5,841.78 |    0.7% |      6.87 | `2 KB writes`
|              113.33 |            8,823.41 |    1.6% |      6.86 | `4 KB writes`
|               86.59 |           11,549.31 |    2.4% |      7.01 | `8 KB writes`
|               84.91 |           11,777.48 |    1.7% |      6.93 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.49 GB | `1 KB writes`
|            39.31 GB | `2 KB writes`
|            59.44 GB | `4 KB writes`
|            79.02 GB | `8 KB writes`
|            81.17 GB | `16 KB writes`
