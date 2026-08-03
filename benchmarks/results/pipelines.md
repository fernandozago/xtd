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
|              285.50 |            3,502.68 |    1.4% |      6.88 | `1 KB writes`
|              173.03 |            5,779.20 |    2.4% |      6.91 | `2 KB writes`
|              122.35 |            8,173.14 |    3.1% |      6.74 | `4 KB writes`
|               87.39 |           11,442.57 |    2.0% |      6.90 | `8 KB writes`
|               82.57 |           12,110.83 |    1.0% |      6.86 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.60 GB | `1 KB writes`
|            39.09 GB | `2 KB writes`
|            54.50 GB | `4 KB writes`
|            77.80 GB | `8 KB writes`
|            82.53 GB | `16 KB writes`
