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
|              280.80 |            3,561.19 |    2.0% |      6.98 | `1 KB writes`
|              159.49 |            6,270.14 |    1.2% |      6.92 | `2 KB writes`
|              112.13 |            8,918.27 |    1.5% |      6.80 | `4 KB writes`
|               88.29 |           11,326.56 |    1.0% |      6.94 | `8 KB writes`
|               82.99 |           12,050.34 |    1.5% |      6.93 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.53 GB | `1 KB writes`
|            42.36 GB | `2 KB writes`
|            59.33 GB | `4 KB writes`
|            77.37 GB | `8 KB writes`
|            82.58 GB | `16 KB writes`
