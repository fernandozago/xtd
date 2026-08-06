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
|                1.09 |              914.82 |    0.8% |      9.81 | `fixed_pool_resource / allocate+deallocate(1)`
|                0.83 |            1,207.93 |    0.3% |      9.91 | `arena_pool_resource / allocate+deallocate(1)`
|                6.11 |              163.73 |    0.4% |      9.81 | `unsynchronized_pool_resource / allocate+deallocate(1)`
|                1.52 |              659.29 |    0.2% |      9.89 | `fixed_pool_resource / deque-fifo-burst(32)`
|                1.44 |              694.37 |    0.2% |      9.93 | `arena_pool_resource / deque-fifo-burst(32)`
|                6.74 |              148.36 |    0.2% |      9.90 | `unsynchronized_pool_resource / deque-fifo-burst(32)`
