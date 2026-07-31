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
|              262.38 |            3,811.24 |    2.5% |      6.83 | `1 KB writes`
|              165.85 |            6,029.43 |    0.8% |      6.83 | `2 KB writes`
|              122.53 |            8,161.31 |    1.8% |      6.79 | `4 KB writes`
|               86.63 |           11,543.21 |    1.2% |      6.89 | `8 KB writes`
|               79.37 |           12,599.69 |    1.9% |      6.81 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.41 GB | `1 KB writes`
|            40.59 GB | `2 KB writes`
|            54.81 GB | `4 KB writes`
|            78.43 GB | `8 KB writes`
|            85.54 GB | `16 KB writes`
