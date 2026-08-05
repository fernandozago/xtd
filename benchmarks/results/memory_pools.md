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
|                1.04 |              964.54 |    0.7% |      9.96 | `fixed_pool_resource / allocate+deallocate(1)`
|                0.92 |            1,084.67 |    0.3% |      9.95 | `arena_pool_resource / allocate+deallocate(1)`
|                5.92 |              168.98 |    0.7% |      9.86 | `unsynchronized_pool_resource / allocate+deallocate(1)`
|                1.47 |              680.52 |    0.6% |      9.90 | `fixed_pool_resource / deque-fifo-burst(32)`
|                1.40 |              712.08 |    0.4% |      9.93 | `arena_pool_resource / deque-fifo-burst(32)`
|                6.67 |              149.98 |    0.9% |      9.94 | `unsynchronized_pool_resource / deque-fifo-burst(32)`
