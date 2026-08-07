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
|              279.33 |            3,579.93 |    1.1% |      6.86 | `fixed_pool_resource / 1 KB writes`
|              323.22 |            3,093.84 |    0.6% |      6.91 | `arena_pool_resource / 1 KB writes`
|              177.05 |            5,648.11 |    1.2% |      6.79 | `fixed_pool_resource / 2 KB writes`
|              196.73 |            5,083.21 |    1.1% |      6.81 | `arena_pool_resource / 2 KB writes`
|              124.01 |            8,063.60 |    1.1% |      6.83 | `fixed_pool_resource / 4 KB writes`
|              129.29 |            7,734.66 |    1.9% |      6.88 | `arena_pool_resource / 4 KB writes`
|               86.52 |           11,558.19 |    0.9% |      6.82 | `fixed_pool_resource / 8 KB writes`
|               84.30 |           11,862.52 |    1.0% |      6.95 | `arena_pool_resource / 8 KB writes`
|               80.32 |           12,449.47 |    0.6% |      6.89 | `fixed_pool_resource / 16 KB writes`
|               79.67 |           12,552.28 |    1.3% |      6.86 | `arena_pool_resource / 16 KB writes`
|               81.57 |           12,258.94 |    1.6% |      6.96 | `fixed_pool_resource / 32 KB writes`
|               85.12 |           11,747.97 |    1.0% |      6.92 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.03 GB | `fixed_pool_resource / 1 KB writes`
|            21.02 GB | `arena_pool_resource / 1 KB writes`
|            37.56 GB | `fixed_pool_resource / 2 KB writes`
|            34.11 GB | `arena_pool_resource / 2 KB writes`
|            54.24 GB | `fixed_pool_resource / 4 KB writes`
|            52.77 GB | `arena_pool_resource / 4 KB writes`
|            77.89 GB | `fixed_pool_resource / 8 KB writes`
|            81.06 GB | `arena_pool_resource / 8 KB writes`
|            85.36 GB | `fixed_pool_resource / 16 KB writes`
|            86.16 GB | `arena_pool_resource / 16 KB writes`
|            83.07 GB | `fixed_pool_resource / 32 KB writes`
|            80.00 GB | `arena_pool_resource / 32 KB writes`
