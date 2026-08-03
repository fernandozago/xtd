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
|              267.45 |            3,738.99 |    1.4% |      6.79 | `1 KB writes`
|              156.80 |            6,377.44 |    1.6% |      6.90 | `2 KB writes`
|              113.92 |            8,778.23 |    1.3% |      6.90 | `4 KB writes`
|               81.49 |           12,272.13 |    0.7% |      6.84 | `8 KB writes`
|               79.70 |           12,546.54 |    1.0% |      6.80 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.59 GB | `1 KB writes`
|            43.13 GB | `2 KB writes`
|            59.48 GB | `4 KB writes`
|            82.52 GB | `8 KB writes`
|            85.11 GB | `16 KB writes`
