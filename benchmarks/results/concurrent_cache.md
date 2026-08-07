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
|               33.66 |       29,707,756.79 |    4.3% |     11.55 | `insert_or_assign`
|               56.92 |       17,567,237.61 |    1.7% |      9.06 | `get_or_create(ttl: 0ms)`
|              494.52 |        2,022,145.57 |    1.0% |     11.53 | `get_or_create(ttl: 1ms)`
|               51.14 |       19,552,314.45 |    0.8% |     11.52 | `get / pre-populated-cache`
