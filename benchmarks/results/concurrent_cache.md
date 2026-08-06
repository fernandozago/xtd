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
|               36.32 |       27,533,040.67 |    2.2% |     11.40 | `insert_or_assign`
|               56.67 |       17,646,442.64 |    0.5% |      9.26 | `get_or_create(ttl: 0ms)`
|              489.98 |        2,040,890.67 |    2.5% |     11.60 | `get_or_create(ttl: 1ms)`
|               49.16 |       20,341,891.28 |    0.8% |     11.54 | `get / pre-populated-cache`
