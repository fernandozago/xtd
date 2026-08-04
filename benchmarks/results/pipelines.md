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
|              284.84 |            3,510.73 |    1.7% |      6.76 | `1 KB writes`
|              172.40 |            5,800.45 |    1.3% |      6.82 | `2 KB writes`
|              115.28 |            8,674.74 |    1.5% |      6.87 | `4 KB writes`
|               83.89 |           11,920.61 |    1.9% |      6.96 | `8 KB writes`
|               81.12 |           12,328.12 |    1.1% |      6.92 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.49 GB | `1 KB writes`
|            39.12 GB | `2 KB writes`
|            58.67 GB | `4 KB writes`
|            81.83 GB | `8 KB writes`
|            84.75 GB | `16 KB writes`
