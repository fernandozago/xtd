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
|              288.14 |            3,470.53 |    1.4% |      6.91 | `1 KB writes`
|              171.15 |            5,842.76 |    1.0% |      6.86 | `2 KB writes`
|              119.36 |            8,377.73 |    2.7% |      6.88 | `4 KB writes`
|               84.43 |           11,843.48 |    1.1% |      6.81 | `8 KB writes`
|               82.83 |           12,073.44 |    1.6% |      6.93 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.70 GB | `1 KB writes`
|            39.33 GB | `2 KB writes`
|            56.88 GB | `4 KB writes`
|            79.73 GB | `8 KB writes`
|            83.32 GB | `16 KB writes`
