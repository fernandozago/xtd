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
|               36.36 |       27,501,353.04 |    3.1% |     11.63 | `insert_or_assign`
|               57.00 |       17,544,313.67 |    1.0% |      9.11 | `get_or_create(ttl: 0ms)`
|              503.23 |        1,987,153.60 |    0.5% |     11.50 | `get_or_create(ttl: 1ms)`
|               51.35 |       19,475,716.60 |    1.0% |     11.56 | `get / pre-populated-cache`
