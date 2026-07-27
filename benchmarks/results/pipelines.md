Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* CPU governor is 'powersave' but should be 'performance'
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|              μs/MB |                MB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              270.65 |            3,694.76 |    0.7% |      6.86 | `1 KB writes`
|              161.30 |            6,199.72 |    1.0% |      6.86 | `2 KB writes`
|              109.37 |            9,143.50 |    0.6% |      6.88 | `4 KB writes`
|               80.99 |           12,347.27 |    1.7% |      6.84 | `8 KB writes`
|               79.14 |           12,635.33 |    1.8% |      6.92 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.76 GB | `1 KB writes`
|            41.86 GB | `2 KB writes`
|            62.07 GB | `4 KB writes`
|            83.68 GB | `8 KB writes`
|            86.78 GB | `16 KB writes`
