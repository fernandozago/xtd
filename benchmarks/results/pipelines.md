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
|              253.98 |            3,937.25 |    0.9% |      6.91 | `1 KB writes`
|              162.54 |            6,152.38 |    0.6% |      6.83 | `2 KB writes`
|              124.20 |            8,051.73 |    0.7% |      6.86 | `4 KB writes`
|               87.89 |           11,377.33 |    1.1% |      6.86 | `8 KB writes`
|               78.14 |           12,797.25 |    1.2% |      6.80 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            26.62 GB | `1 KB writes`
|            40.91 GB | `2 KB writes`
|            54.43 GB | `4 KB writes`
|            77.16 GB | `8 KB writes`
|            85.71 GB | `16 KB writes`
