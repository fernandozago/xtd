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
|              271.97 |            3,676.83 |    1.5% |      6.77 | `fixed_pool_resource / 1 KB writes`
|              308.40 |            3,242.55 |    0.9% |      6.90 | `arena_pool_resource / 1 KB writes`
|              175.03 |            5,713.30 |    1.0% |      6.87 | `fixed_pool_resource / 2 KB writes`
|              199.82 |            5,004.50 |    1.8% |      6.83 | `arena_pool_resource / 2 KB writes`
|              130.31 |            7,674.05 |    3.2% |      7.09 | `fixed_pool_resource / 4 KB writes`
|              136.47 |            7,327.72 |    0.5% |      6.89 | `arena_pool_resource / 4 KB writes`
|               92.07 |           10,861.08 |    2.1% |      6.91 | `fixed_pool_resource / 8 KB writes`
|               89.56 |           11,165.36 |    0.6% |      6.84 | `arena_pool_resource / 8 KB writes`
|               86.58 |           11,549.37 |    1.8% |      6.94 | `fixed_pool_resource / 16 KB writes`
|               89.12 |           11,220.53 |    1.2% |      6.89 | `arena_pool_resource / 16 KB writes`
|               87.24 |           11,463.15 |    1.3% |      6.95 | `fixed_pool_resource / 32 KB writes`
|               91.80 |           10,893.57 |    2.6% |      6.87 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.47 GB | `fixed_pool_resource / 1 KB writes`
|            21.90 GB | `arena_pool_resource / 1 KB writes`
|            38.56 GB | `fixed_pool_resource / 2 KB writes`
|            33.51 GB | `arena_pool_resource / 2 KB writes`
|            52.56 GB | `fixed_pool_resource / 4 KB writes`
|            49.67 GB | `arena_pool_resource / 4 KB writes`
|            74.06 GB | `fixed_pool_resource / 8 KB writes`
|            75.36 GB | `arena_pool_resource / 8 KB writes`
|            78.98 GB | `fixed_pool_resource / 16 KB writes`
|            77.04 GB | `arena_pool_resource / 16 KB writes`
|            77.11 GB | `fixed_pool_resource / 32 KB writes`
|            73.19 GB | `arena_pool_resource / 32 KB writes`
