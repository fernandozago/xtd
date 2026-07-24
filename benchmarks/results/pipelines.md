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
|              271.79 |            3,679.26 |    1.7% |      5.45 | `1 KiB writes`
|              167.02 |            5,987.44 |    1.4% |      5.54 | `2 KiB writes`
|              120.92 |            8,269.97 |    2.3% |      5.52 | `4 KiB writes`
|               86.27 |           11,591.55 |    0.8% |      5.42 | `8 KiB writes`
|               80.71 |           12,389.77 |    1.6% |      5.52 | `16 KiB writes`
|               79.97 |           12,504.93 |    0.8% |      5.47 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           19.73 GiB | 1 KiB writes
|           32.47 GiB | 2 KiB writes
|           44.90 GiB | 4 KiB writes
|           61.81 GiB | 8 KiB writes
|           67.87 GiB | 16 KiB writes
|           67.23 GiB | 32 KiB writes
