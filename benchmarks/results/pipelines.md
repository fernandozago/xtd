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
|              269.76 |            3,707.01 |    2.1% |      6.95 | `1 KB writes`
|              168.41 |            5,937.92 |    1.0% |      6.81 | `2 KB writes`
|              113.07 |            8,844.39 |    0.8% |      6.88 | `4 KB writes`
|               83.99 |           11,906.36 |    2.6% |      6.91 | `8 KB writes`
|               80.47 |           12,427.43 |    2.7% |      7.01 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.19 GB | `1 KB writes`
|            39.39 GB | `2 KB writes`
|            59.86 GB | `4 KB writes`
|            81.43 GB | `8 KB writes`
|            85.58 GB | `16 KB writes`
