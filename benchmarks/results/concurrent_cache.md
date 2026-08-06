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
|               33.38 |       29,959,723.95 |    0.7% |     11.64 | `insert_or_assign`
|               56.95 |       17,559,574.18 |    0.6% |      9.24 | `get_or_create(ttl: 0ms)`
|              497.32 |        2,010,779.39 |    0.3% |     11.33 | `get_or_create(ttl: 1ms)`
|               50.52 |       19,795,398.05 |    0.3% |     11.53 | `get / pre-populated-cache`
