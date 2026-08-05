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
|              289.81 |            3,450.48 |    0.5% |      6.88 | `fixed_pool_resource / 1 KB writes`
|              324.10 |            3,085.45 |    0.7% |      6.83 | `arena_pool_resource / 1 KB writes`
|              180.46 |            5,541.31 |    1.9% |      6.76 | `fixed_pool_resource / 2 KB writes`
|              191.36 |            5,225.69 |    0.7% |      6.90 | `arena_pool_resource / 2 KB writes`
|              136.44 |            7,329.25 |    3.5% |      6.76 | `fixed_pool_resource / 4 KB writes`
|              133.99 |            7,463.43 |    1.6% |      6.95 | `arena_pool_resource / 4 KB writes`
|               95.42 |           10,480.16 |    2.1% |      6.95 | `fixed_pool_resource / 8 KB writes`
|               92.31 |           10,833.50 |    1.2% |      6.82 | `arena_pool_resource / 8 KB writes`
|               87.18 |           11,470.50 |    1.3% |      6.87 | `fixed_pool_resource / 16 KB writes`
|               84.94 |           11,773.23 |    1.1% |      6.88 | `arena_pool_resource / 16 KB writes`
|               89.88 |           11,125.85 |    1.0% |      6.97 | `fixed_pool_resource / 32 KB writes`
|               88.07 |           11,354.25 |    0.9% |      7.03 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.33 GB | `fixed_pool_resource / 1 KB writes`
|            20.65 GB | `arena_pool_resource / 1 KB writes`
|            36.82 GB | `fixed_pool_resource / 2 KB writes`
|            35.35 GB | `arena_pool_resource / 2 KB writes`
|            48.68 GB | `fixed_pool_resource / 4 KB writes`
|            51.19 GB | `arena_pool_resource / 4 KB writes`
|            72.01 GB | `fixed_pool_resource / 8 KB writes`
|            72.87 GB | `arena_pool_resource / 8 KB writes`
|            78.29 GB | `fixed_pool_resource / 16 KB writes`
|            81.35 GB | `arena_pool_resource / 16 KB writes`
|            76.77 GB | `fixed_pool_resource / 32 KB writes`
|            77.07 GB | `arena_pool_resource / 32 KB writes`
