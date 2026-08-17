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
|              284.31 |            3,517.32 |    1.1% |      6.86 | `fixed_pool_resource / 1 KB writes`
|              322.01 |            3,105.54 |    1.7% |      6.84 | `arena_pool_resource / 1 KB writes`
|              188.06 |            5,317.44 |    1.4% |      6.93 | `fixed_pool_resource / 2 KB writes`
|              210.66 |            4,746.92 |    1.1% |      6.84 | `arena_pool_resource / 2 KB writes`
|              123.82 |            8,076.53 |    1.2% |      6.88 | `fixed_pool_resource / 4 KB writes`
|              118.06 |            8,470.22 |    0.9% |      6.72 | `arena_pool_resource / 4 KB writes`
|               89.99 |           11,111.93 |    1.6% |      6.86 | `fixed_pool_resource / 8 KB writes`
|               89.81 |           11,134.97 |    1.9% |      7.00 | `arena_pool_resource / 8 KB writes`
|               87.19 |           11,468.60 |    1.8% |      7.01 | `fixed_pool_resource / 16 KB writes`
|               85.99 |           11,629.69 |    1.5% |      6.69 | `arena_pool_resource / 16 KB writes`
|               85.91 |           11,640.06 |    1.5% |      6.91 | `fixed_pool_resource / 32 KB writes`
|               85.41 |           11,708.58 |    1.4% |      6.93 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.65 GB | `fixed_pool_resource / 1 KB writes`
|            20.84 GB | `arena_pool_resource / 1 KB writes`
|            36.12 GB | `fixed_pool_resource / 2 KB writes`
|            31.94 GB | `arena_pool_resource / 2 KB writes`
|            54.94 GB | `fixed_pool_resource / 4 KB writes`
|            55.13 GB | `arena_pool_resource / 4 KB writes`
|            75.31 GB | `fixed_pool_resource / 8 KB writes`
|            77.25 GB | `arena_pool_resource / 8 KB writes`
|            79.66 GB | `fixed_pool_resource / 16 KB writes`
|            76.97 GB | `arena_pool_resource / 16 KB writes`
|            79.04 GB | `fixed_pool_resource / 32 KB writes`
|            79.38 GB | `arena_pool_resource / 32 KB writes`
