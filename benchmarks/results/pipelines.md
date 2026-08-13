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
|              299.76 |            3,335.98 |    1.2% |      6.86 | `fixed_pool_resource / 1 KB writes`
|              334.63 |            2,988.42 |    1.2% |      6.84 | `arena_pool_resource / 1 KB writes`
|              191.06 |            5,234.04 |    1.0% |      6.87 | `fixed_pool_resource / 2 KB writes`
|              202.37 |            4,941.36 |    1.0% |      6.85 | `arena_pool_resource / 2 KB writes`
|              131.45 |            7,607.31 |    2.4% |      6.99 | `fixed_pool_resource / 4 KB writes`
|              130.86 |            7,641.51 |    1.8% |      6.82 | `arena_pool_resource / 4 KB writes`
|               94.52 |           10,580.23 |    0.8% |      6.84 | `fixed_pool_resource / 8 KB writes`
|               91.44 |           10,936.38 |    1.2% |      7.00 | `arena_pool_resource / 8 KB writes`
|               88.42 |           11,309.51 |    1.5% |      6.87 | `fixed_pool_resource / 16 KB writes`
|               87.15 |           11,475.06 |    0.9% |      6.86 | `arena_pool_resource / 16 KB writes`
|               91.14 |           10,971.99 |    1.2% |      6.96 | `fixed_pool_resource / 32 KB writes`
|               88.83 |           11,256.91 |    1.7% |      6.92 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            22.53 GB | `fixed_pool_resource / 1 KB writes`
|            20.01 GB | `arena_pool_resource / 1 KB writes`
|            35.38 GB | `fixed_pool_resource / 2 KB writes`
|            33.29 GB | `arena_pool_resource / 2 KB writes`
|            51.96 GB | `fixed_pool_resource / 4 KB writes`
|            50.68 GB | `arena_pool_resource / 4 KB writes`
|            71.91 GB | `fixed_pool_resource / 8 KB writes`
|            75.02 GB | `arena_pool_resource / 8 KB writes`
|            76.52 GB | `fixed_pool_resource / 16 KB writes`
|            78.28 GB | `arena_pool_resource / 16 KB writes`
|            74.30 GB | `fixed_pool_resource / 32 KB writes`
|            76.00 GB | `arena_pool_resource / 32 KB writes`
