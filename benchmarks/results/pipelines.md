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
|              349.99 |            2,857.19 |    2.5% |      6.80 | `1 KB writes`
arena_pool_resource usage:
  buffer size:         4096
  arena capacity:      32
  arena bytes:         131072
  total allocations:   5352093
  total deallocations: 5352093
  reused buffers:      5352061
  failed allocations:  0
  peak active:         32
  active at shutdown:  0
|              218.59 |            4,574.76 |    1.9% |      6.89 | `2 KB writes`
arena_pool_resource usage:
  buffer size:         4096
  arena capacity:      32
  arena bytes:         131072
  total allocations:   8341480
  total deallocations: 8341480
  reused buffers:      8341448
  failed allocations:  0
  peak active:         32
  active at shutdown:  0
|              129.23 |            7,737.85 |    1.1% |      6.84 | `4 KB writes`
arena_pool_resource usage:
  buffer size:         4096
  arena capacity:      32
  arena bytes:         131072
  total allocations:   13698533
  total deallocations: 13698533
  reused buffers:      13698501
  failed allocations:  0
  peak active:         32
  active at shutdown:  0
|               92.74 |           10,782.73 |    1.2% |      6.94 | `8 KB writes`
arena_pool_resource usage:
  buffer size:         4096
  arena capacity:      32
  arena bytes:         131072
  total allocations:   19477426
  total deallocations: 19477426
  reused buffers:      19477394
  failed allocations:  0
  peak active:         32
  active at shutdown:  0
|               86.84 |           11,514.80 |    1.4% |      6.88 | `16 KB writes`
arena_pool_resource usage:
  buffer size:         4096
  arena capacity:      32
  arena bytes:         131072
  total allocations:   20697984
  total deallocations: 20697984
  reused buffers:      20697952
  failed allocations:  0
  peak active:         32
  active at shutdown:  0

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            18.86 GB | `1 KB writes`
|            31.13 GB | `2 KB writes`
|            52.26 GB | `4 KB writes`
|            74.30 GB | `8 KB writes`
|            78.96 GB | `16 KB writes`
