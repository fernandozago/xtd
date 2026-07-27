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
|              264.63 |            3,778.82 |    1.7% |      6.78 | `1 KB writes`
|              159.91 |            6,253.59 |    1.2% |      6.89 | `2 KB writes`
|              112.91 |            8,856.60 |    1.0% |      6.84 | `4 KB writes`
|               80.93 |           12,356.73 |    1.4% |      6.77 | `8 KB writes`
|               78.97 |           12,662.81 |    1.6% |      6.84 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.23 GB | `1 KB writes`
|            42.29 GB | `2 KB writes`
|            59.40 GB | `4 KB writes`
|            81.66 GB | `8 KB writes`
|            85.51 GB | `16 KB writes`
