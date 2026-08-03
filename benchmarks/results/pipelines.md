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
|              271.62 |            3,681.59 |    0.9% |      6.94 | `1 KB writes`
|              162.71 |            6,145.92 |    1.3% |      6.91 | `2 KB writes`
|              104.52 |            9,567.98 |    1.3% |      6.79 | `4 KB writes`
|               76.08 |           13,143.45 |    2.3% |      6.91 | `8 KB writes`
|               80.53 |           12,418.31 |    0.7% |      6.95 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.99 GB | `1 KB writes`
|            41.85 GB | `2 KB writes`
|            63.78 GB | `4 KB writes`
|            88.18 GB | `8 KB writes`
|            86.26 GB | `16 KB writes`
