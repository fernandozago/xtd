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
|              263.34 |            3,797.44 |    1.0% |      6.86 | `1 KB writes`
|              160.49 |            6,230.81 |    1.5% |      6.92 | `2 KB writes`
|              113.23 |            8,831.51 |    0.9% |      6.85 | `4 KB writes`
|               82.42 |           12,133.22 |    2.3% |      6.88 | `8 KB writes`
|               78.87 |           12,679.01 |    1.4% |      6.89 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.62 GB | `1 KB writes`
|            42.21 GB | `2 KB writes`
|            59.32 GB | `4 KB writes`
|            82.17 GB | `8 KB writes`
|            86.78 GB | `16 KB writes`
