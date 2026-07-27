Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* CPU governor is 'powersave' but should be 'performance'
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|              μs/MB |                MB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              296.99 |            3,367.15 |    1.1% |      6.86 | `1 KB writes`
|              192.53 |            5,193.98 |    3.0% |      6.58 | `2 KB writes`
|              126.27 |            7,919.82 |    3.7% |      6.71 | `4 KB writes`
|               85.17 |           11,741.85 |    1.0% |      6.87 | `8 KB writes`
|               78.90 |           12,674.67 |    0.6% |      6.82 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            22.55 GB | `1 KB writes`
|            31.68 GB | `2 KB writes`
|            52.68 GB | `4 KB writes`
|            79.65 GB | `8 KB writes`
|            85.21 GB | `16 KB writes`
