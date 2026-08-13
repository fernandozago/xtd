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
|                1.11 |              903.36 |    0.4% |      9.93 | `fixed_pool_resource / allocate+deallocate(1)`
|                0.84 |            1,183.75 |    0.7% |      9.92 | `arena_pool_resource / allocate+deallocate(1)`
|                6.09 |              164.08 |    0.4% |      9.81 | `unsynchronized_pool_resource / allocate+deallocate(1)`
|                1.50 |              665.59 |    0.3% |      9.92 | `fixed_pool_resource / deque-fifo-burst(32)`
|                1.45 |              691.81 |    0.5% |      9.89 | `arena_pool_resource / deque-fifo-burst(32)`
|                6.84 |              146.15 |    0.5% |      9.93 | `unsynchronized_pool_resource / deque-fifo-burst(32)`
