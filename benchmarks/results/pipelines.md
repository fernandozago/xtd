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
|              284.52 |            3,514.74 |    0.4% |      6.85 | `1 KB writes`
|              163.68 |            6,109.51 |    1.2% |      6.83 | `2 KB writes`
|              118.15 |            8,463.69 |    1.1% |      6.91 | `4 KB writes`
|               85.51 |           11,694.44 |    1.1% |      6.89 | `8 KB writes`
|               82.41 |           12,134.86 |    0.9% |      6.90 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.56 GB | `1 KB writes`
|            40.20 GB | `2 KB writes`
|            57.66 GB | `4 KB writes`
|            79.80 GB | `8 KB writes`
|            83.53 GB | `16 KB writes`
