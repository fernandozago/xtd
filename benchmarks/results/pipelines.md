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
|              273.54 |            3,655.76 |    1.4% |      6.98 | `1 KB writes`
|              155.29 |            6,439.44 |    2.6% |      6.85 | `2 KB writes`
|              102.87 |            9,721.28 |    0.8% |      6.83 | `4 KB writes`
|               83.99 |           11,906.03 |    0.7% |      7.00 | `8 KB writes`
|               83.24 |           12,013.04 |    1.8% |      6.89 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.23 GB | `1 KB writes`
|            43.43 GB | `2 KB writes`
|            65.33 GB | `4 KB writes`
|            82.16 GB | `8 KB writes`
|            82.78 GB | `16 KB writes`
