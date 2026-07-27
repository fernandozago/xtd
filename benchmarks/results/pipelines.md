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
|              268.60 |            3,723.06 |    1.0% |      6.86 | `1 KB writes`
|              159.41 |            6,272.98 |    1.3% |      6.82 | `2 KB writes`
|              110.15 |            9,078.64 |    4.7% |      7.29 | `4 KB writes`
|               87.18 |           11,470.68 |    2.1% |      6.98 | `8 KB writes`
|               78.51 |           12,737.63 |    1.3% |      6.90 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.95 GB | `1 KB writes`
|            41.92 GB | `2 KB writes`
|            62.59 GB | `4 KB writes`
|            78.76 GB | `8 KB writes`
|            87.26 GB | `16 KB writes`
