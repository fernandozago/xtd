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
|              297.83 |            3,357.59 |    2.0% |    111.82 | `fixed_pool_resource / 1 KB writes`
|              340.02 |            2,941.02 |    1.5% |    111.10 | `arena_pool_resource / 1 KB writes`
|              193.70 |            5,162.70 |    1.6% |    108.69 | `fixed_pool_resource / 2 KB writes`
|              219.25 |            4,560.90 |    3.1% |    110.48 | `arena_pool_resource / 2 KB writes`
|              132.69 |            7,536.17 |    2.6% |    110.36 | `fixed_pool_resource / 4 KB writes`
|              139.18 |            7,185.17 |    1.5% |    111.93 | `arena_pool_resource / 4 KB writes`
|               97.48 |           10,258.37 |    0.9% |    109.60 | `fixed_pool_resource / 8 KB writes`
|               95.63 |           10,456.84 |    1.4% |    111.22 | `arena_pool_resource / 8 KB writes`
|               89.47 |           11,177.12 |    0.7% |    109.43 | `fixed_pool_resource / 16 KB writes`
|               89.96 |           11,116.19 |    1.1% |    109.66 | `arena_pool_resource / 16 KB writes`
|               88.47 |           11,302.95 |    2.4% |    111.87 | `fixed_pool_resource / 32 KB writes`
|               91.83 |           10,889.96 |    0.9% |    109.89 | `arena_pool_resource / 32 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           369.14 GB | `fixed_pool_resource / 1 KB writes`
|           318.83 GB | `arena_pool_resource / 1 KB writes`
|           545.38 GB | `fixed_pool_resource / 2 KB writes`
|           494.47 GB | `arena_pool_resource / 2 KB writes`
|           817.96 GB | `fixed_pool_resource / 4 KB writes`
|           795.04 GB | `arena_pool_resource / 4 KB writes`
|          1108.84 GB | `fixed_pool_resource / 8 KB writes`
|          1145.75 GB | `arena_pool_resource / 8 KB writes`
|          1196.39 GB | `fixed_pool_resource / 16 KB writes`
|          1190.68 GB | `arena_pool_resource / 16 KB writes`
|          1227.85 GB | `fixed_pool_resource / 32 KB writes`
|          1167.51 GB | `arena_pool_resource / 32 KB writes`
