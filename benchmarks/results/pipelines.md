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
|              263.62 |            3,793.32 |    0.2% |      6.94 | `1 KB writes`
|              169.99 |            5,882.78 |    0.8% |      6.85 | `2 KB writes`
|              115.40 |            8,665.75 |    0.5% |      6.90 | `4 KB writes`
|               80.74 |           12,385.49 |    1.2% |      6.86 | `8 KB writes`
|               79.80 |           12,530.98 |    3.2% |      6.98 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.50 GB | `1 KB writes`
|            39.38 GB | `2 KB writes`
|            58.90 GB | `4 KB writes`
|            83.69 GB | `8 KB writes`
|            87.16 GB | `16 KB writes`
