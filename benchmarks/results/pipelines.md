Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* CPU governor is 'powersave' but should be 'performance'
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|             μs/MiB |               MiB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              266.60 |            3,750.89 |    0.7% |      5.53 | `1 KiB writes`
|              161.79 |            6,180.98 |    1.3% |      5.52 | `2 KiB writes`
|              111.36 |            8,979.89 |    0.6% |      5.47 | `4 KiB writes`
|               80.21 |           12,467.16 |    2.1% |      5.53 | `8 KiB writes`
|               80.39 |           12,439.69 |    1.0% |      5.55 | `16 KiB writes`
|               83.25 |           12,012.27 |    1.5% |      5.52 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           20.13 GiB | 1 KiB writes
|           33.51 GiB | 2 KiB writes
|           48.17 GiB | 4 KiB writes
|           68.46 GiB | 8 KiB writes
|           68.45 GiB | 16 KiB writes
|           64.62 GiB | 32 KiB writes
