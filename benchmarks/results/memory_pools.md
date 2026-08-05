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
|                1.05 |              950.70 |    0.4% |      9.73 | `fixed_pool_resource / allocate+deallocate(1)`
|                0.80 |            1,242.87 |    0.2% |      9.91 | `arena_pool_resource / allocate+deallocate(1)`
|                6.08 |              164.51 |    1.2% |      9.92 | `unsynchronized_pool_resource / allocate+deallocate(1)`
|                1.51 |              663.30 |    0.2% |      9.91 | `fixed_pool_resource / deque-fifo-burst(32)`
|                1.42 |              703.69 |    0.2% |      9.93 | `arena_pool_resource / deque-fifo-burst(32)`
|                6.75 |              148.17 |    0.2% |      9.91 | `unsynchronized_pool_resource / deque-fifo-burst(32)`
