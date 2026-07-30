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
|              277.89 |            3,598.55 |    1.3% |      6.88 | `1 KB writes`
|              165.93 |            6,026.59 |    0.7% |      6.87 | `2 KB writes`
|              119.33 |            8,379.82 |    2.1% |      6.87 | `4 KB writes`
|               87.12 |           11,478.70 |    0.6% |      6.94 | `8 KB writes`
|               83.50 |           11,975.53 |    2.4% |      6.99 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.28 GB | `1 KB writes`
|            40.54 GB | `2 KB writes`
|            56.60 GB | `4 KB writes`
|            78.20 GB | `8 KB writes`
|            83.92 GB | `16 KB writes`
