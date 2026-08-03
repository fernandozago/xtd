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
|              276.23 |            3,620.14 |    1.3% |      6.87 | `1 KB writes`
|              168.42 |            5,937.55 |    1.9% |      6.91 | `2 KB writes`
|              118.70 |            8,424.51 |    0.9% |      6.84 | `4 KB writes`
|               86.27 |           11,591.85 |    1.2% |      7.03 | `8 KB writes`
|               80.53 |           12,417.32 |    1.0% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.18 GB | `1 KB writes`
|            40.15 GB | `2 KB writes`
|            56.68 GB | `4 KB writes`
|            80.93 GB | `8 KB writes`
|            84.99 GB | `16 KB writes`
