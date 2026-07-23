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
|              287.85 |            3,474.03 |    0.9% |      5.50 | `1 KiB writes`
|              169.09 |            5,914.10 |    1.1% |      5.45 | `2 KiB writes`
|              108.99 |            9,175.40 |    1.4% |      5.44 | `4 KiB writes`
|               82.29 |           12,151.56 |    1.5% |      5.49 | `8 KiB writes`
|               77.86 |           12,843.28 |    1.4% |      5.56 | `16 KiB writes`
|               81.25 |           12,307.24 |    1.3% |      5.48 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           18.80 GiB | 1 KiB writes
|           31.62 GiB | 2 KiB writes
|           49.17 GiB | 4 KiB writes
|           66.48 GiB | 8 KiB writes
|           70.68 GiB | 16 KiB writes
|           66.70 GiB | 32 KiB writes
