Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|            ns/items |             items/s |    err% |     total | xtd::concurrent_cache throughput
|--------------------:|--------------------:|--------:|----------:|:---------------------------------
|               36.72 |       27,234,047.35 |    0.8% |     11.55 | `insert_or_assign`
|               59.31 |       16,860,048.32 |    0.8% |      9.03 | `get_or_create(ttl: 0ms)`
|              535.96 |        1,865,803.70 |    1.3% |     11.52 | `get_or_create(ttl: 1ms)`
|               52.00 |       19,230,995.94 |    1.0% |     11.52 | `get / pre-populated-cache`
