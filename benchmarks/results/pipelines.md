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
|              266.45 |            3,753.03 |    0.8% |      6.87 | `1 KB writes`
|              162.45 |            6,155.91 |    2.6% |      6.83 | `2 KB writes`
|              118.29 |            8,454.04 |    1.1% |      6.83 | `4 KB writes`
|               84.27 |           11,866.98 |    0.9% |      6.85 | `8 KB writes`
|               78.60 |           12,723.37 |    1.6% |      6.91 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.10 GB | `1 KB writes`
|            41.32 GB | `2 KB writes`
|            57.01 GB | `4 KB writes`
|            80.47 GB | `8 KB writes`
|            87.51 GB | `16 KB writes`
