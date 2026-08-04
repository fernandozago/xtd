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
|              272.28 |            3,672.75 |    1.5% |      6.93 | `1 KB writes`
|              165.76 |            6,032.88 |    1.3% |      6.88 | `2 KB writes`
|              118.57 |            8,433.96 |    4.5% |      6.83 | `4 KB writes`
|               85.10 |           11,750.89 |    1.6% |      6.93 | `8 KB writes`
|               80.33 |           12,448.65 |    0.6% |      6.87 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.97 GB | `1 KB writes`
|            40.42 GB | `2 KB writes`
|            56.72 GB | `4 KB writes`
|            80.63 GB | `8 KB writes`
|            84.75 GB | `16 KB writes`
