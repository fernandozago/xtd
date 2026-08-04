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
|              289.77 |            3,451.03 |    1.6% |      6.77 | `1 KB writes`
|              174.47 |            5,731.74 |    1.9% |      6.96 | `2 KB writes`
|              112.90 |            8,857.23 |    1.4% |      6.90 | `4 KB writes`
|               81.06 |           12,336.75 |    1.4% |      6.98 | `8 KB writes`
|               81.16 |           12,321.74 |    1.2% |      6.82 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.06 GB | `1 KB writes`
|            39.47 GB | `2 KB writes`
|            60.12 GB | `4 KB writes`
|            85.20 GB | `8 KB writes`
|            83.93 GB | `16 KB writes`
