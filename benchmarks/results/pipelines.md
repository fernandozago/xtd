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
|              262.95 |            3,803.04 |    0.5% |      6.82 | `1 KB writes`
|              152.38 |            6,562.72 |    1.3% |      6.86 | `2 KB writes`
|              114.38 |            8,742.70 |    5.1% |      7.11 | :wavy_dash: `4 KB writes` (Unstable with ~612,229.1 iters. Increase `minEpochIterations` to e.g. 6122291)
|               85.72 |           11,666.21 |    2.3% |      6.16 | `8 KB writes`
|               87.09 |           11,482.25 |    5.0% |      5.84 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.25 GB | `1 KB writes`
|            44.14 GB | `2 KB writes`
|            58.81 GB | `4 KB writes`
|            62.51 GB | `8 KB writes`
|            55.40 GB | `16 KB writes`
