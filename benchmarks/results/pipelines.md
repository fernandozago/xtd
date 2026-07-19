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
|              199.26 |            5,018.57 |    1.3% |      5.44 | `1 KiB writes`
|              134.45 |            7,437.55 |    3.6% |      5.61 | `2 KiB writes`
|               94.14 |           10,622.09 |    2.6% |      5.41 | `4 KiB writes`
|               75.14 |           13,309.04 |    2.4% |      5.44 | `8 KiB writes`
|               76.75 |           13,030.08 |    1.0% |      5.44 | `16 KiB writes`
|               82.50 |           12,120.72 |    1.5% |      5.53 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           26.68 GiB | 1 KiB writes
|           41.18 GiB | 2 KiB writes
|           57.16 GiB | 4 KiB writes
|           70.44 GiB | 8 KiB writes
|           71.34 GiB | 16 KiB writes
|           65.78 GiB | 32 KiB writes
