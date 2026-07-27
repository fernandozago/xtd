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
|              285.69 |            3,500.32 |    0.7% |      6.96 | `1 KB writes`
|              171.91 |            5,817.14 |    1.0% |      6.88 | `2 KB writes`
|              112.15 |            8,916.28 |    1.0% |      6.77 | `4 KB writes`
|               83.93 |           11,914.35 |    0.7% |      6.90 | `8 KB writes`
|               78.35 |           12,763.14 |    1.5% |      6.90 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.02 GB | `1 KB writes`
|            39.30 GB | `2 KB writes`
|            59.01 GB | `4 KB writes`
|            80.88 GB | `8 KB writes`
|            87.37 GB | `16 KB writes`
