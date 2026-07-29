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
|              271.31 |            3,685.76 |    3.0% |      6.62 | `1 KB writes`
|              154.90 |            6,455.72 |    1.7% |      6.81 | `2 KB writes`
|              109.09 |            9,166.58 |    1.7% |      6.79 | `4 KB writes`
|               84.97 |           11,769.40 |    0.7% |      6.90 | `8 KB writes`
|               79.92 |           12,512.07 |    0.6% |      6.89 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.57 GB | `1 KB writes`
|            43.26 GB | `2 KB writes`
|            61.11 GB | `4 KB writes`
|            80.22 GB | `8 KB writes`
|            85.35 GB | `16 KB writes`
