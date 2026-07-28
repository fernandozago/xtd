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
|              253.80 |            3,940.14 |    2.5% |      6.76 | `1 KB writes`
|              154.06 |            6,490.85 |    1.2% |      6.84 | `2 KB writes`
|              107.12 |            9,335.17 |    1.2% |      6.82 | `4 KB writes`
|               79.72 |           12,543.31 |    1.7% |      6.93 | `8 KB writes`
|               76.96 |           12,993.78 |    1.3% |      6.81 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            26.44 GB | `1 KB writes`
|            43.58 GB | `2 KB writes`
|            62.46 GB | `4 KB writes`
|            85.53 GB | `8 KB writes`
|            87.36 GB | `16 KB writes`
