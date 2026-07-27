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
|              288.40 |            3,467.46 |    1.9% |      6.95 | `1 KB writes`
|              168.76 |            5,925.70 |    2.1% |      6.68 | `2 KB writes`
|              116.86 |            8,556.94 |    1.3% |      6.95 | `4 KB writes`
|               81.59 |           12,256.85 |    2.6% |      6.99 | `8 KB writes`
|               79.83 |           12,526.69 |    0.9% |      6.82 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.48 GB | `1 KB writes`
|            39.30 GB | `2 KB writes`
|            58.89 GB | `4 KB writes`
|            84.67 GB | `8 KB writes`
|            84.74 GB | `16 KB writes`
