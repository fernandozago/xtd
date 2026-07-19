Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|             μs/MiB |               MiB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              232.40 |            4,302.89 |    1.5% |      5.51 | `1 KiB writes`
|              152.38 |            6,562.33 |    1.1% |      5.44 | `2 KiB writes`
|              104.79 |            9,543.19 |    1.1% |      5.54 | `4 KiB writes`
|               80.85 |           12,368.36 |    0.8% |      5.48 | `8 KiB writes`
|               78.72 |           12,704.01 |    2.3% |      5.46 | `16 KiB writes`
|               79.31 |           12,608.64 |    2.9% |      5.34 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           23.21 GiB | 1 KiB writes
|           35.20 GiB | 2 KiB writes
|           52.07 GiB | 4 KiB writes
|           67.03 GiB | 8 KiB writes
|           69.58 GiB | 16 KiB writes
|           65.10 GiB | 32 KiB writes
