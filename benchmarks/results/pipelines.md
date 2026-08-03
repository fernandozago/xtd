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
|              283.94 |            3,521.85 |    2.2% |      7.10 | `1 KB writes`
|              168.33 |            5,940.89 |    2.4% |      6.76 | `2 KB writes`
|              112.91 |            8,856.49 |    4.4% |      6.90 | `4 KB writes`
|               82.73 |           12,087.16 |    1.6% |      6.91 | `8 KB writes`
|               91.14 |           10,971.57 |    7.1% |      8.97 | :wavy_dash: `16 KB writes` (Unstable with ~164,649.6 iters. Increase `minEpochIterations` to e.g. 1646496)

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.65 GB | `1 KB writes`
|            39.02 GB | `2 KB writes`
|            59.95 GB | `4 KB writes`
|            82.20 GB | `8 KB writes`
|            64.50 GB | `16 KB writes`
