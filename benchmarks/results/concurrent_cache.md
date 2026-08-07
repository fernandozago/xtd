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
|               34.24 |       29,205,480.06 |    2.9% |     11.95 | `insert_or_assign`
|               57.69 |       17,333,767.71 |    3.2% |      8.82 | `get_or_create(ttl: 0ms)`
|              494.57 |        2,021,951.58 |    0.6% |     11.53 | `get_or_create(ttl: 1ms)`
|               51.03 |       19,596,043.80 |    0.5% |     11.52 | `get / pre-populated-cache`
