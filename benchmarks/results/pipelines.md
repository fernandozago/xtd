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
|              268.86 |            3,719.41 |    0.8% |      6.86 | `1 KB writes`
|              162.78 |            6,143.08 |    0.8% |      6.85 | `2 KB writes`
|              114.85 |            8,706.93 |    2.8% |      6.81 | `4 KB writes`
|               86.39 |           11,574.99 |    1.1% |      6.86 | `8 KB writes`
|               80.98 |           12,349.48 |    1.1% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.01 GB | `1 KB writes`
|            41.28 GB | `2 KB writes`
|            58.38 GB | `4 KB writes`
|            78.56 GB | `8 KB writes`
|            84.98 GB | `16 KB writes`
