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
|              276.91 |            3,611.25 |    0.9% |      6.85 | `1 KB writes`
|              168.28 |            5,942.63 |    0.9% |      6.90 | `2 KB writes`
|              115.72 |            8,641.20 |    1.4% |      6.89 | `4 KB writes`
|               81.61 |           12,254.12 |    2.0% |      6.81 | `8 KB writes`
|               78.54 |           12,731.57 |    1.1% |      6.82 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.32 GB | `1 KB writes`
|            40.21 GB | `2 KB writes`
|            58.32 GB | `4 KB writes`
|            81.53 GB | `8 KB writes`
|            86.32 GB | `16 KB writes`
