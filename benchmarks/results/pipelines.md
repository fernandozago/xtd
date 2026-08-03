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
|              286.09 |            3,495.40 |    1.8% |      6.91 | `1 KB writes`
|              169.74 |            5,891.36 |    1.5% |      6.86 | `2 KB writes`
|              120.44 |            8,302.79 |    4.3% |      6.87 | `4 KB writes`
|               84.91 |           11,777.78 |    2.0% |      6.95 | `8 KB writes`
|               83.32 |           12,001.66 |    1.5% |      6.83 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.69 GB | `1 KB writes`
|            39.67 GB | `2 KB writes`
|            56.10 GB | `4 KB writes`
|            80.24 GB | `8 KB writes`
|            81.90 GB | `16 KB writes`
