Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|               ms/GB |                GB/s |    err% |     total | xtd custom allocator throughput
|--------------------:|--------------------:|--------:|----------:|:--------------------------------
|                1.02 |              976.07 |    0.3% |      9.94 | `fixed_pool_resource / allocate+deallocate(1)`
|                0.80 |            1,254.10 |    0.2% |      9.93 | `arena_pool_resource / allocate+deallocate(1)`
|                5.80 |              172.49 |    0.4% |      9.79 | `unsynchronized_pool_resource / allocate+deallocate(1)`
|                1.46 |              687.24 |    0.3% |      9.94 | `fixed_pool_resource / deque-fifo-burst(32)`
|                1.40 |              713.05 |    0.2% |      9.94 | `arena_pool_resource / deque-fifo-burst(32)`
|                6.60 |              151.62 |    0.4% |      9.92 | `unsynchronized_pool_resource / deque-fifo-burst(32)`
