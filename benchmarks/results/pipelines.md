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
|              290.70 |            3,439.98 |    1.0% |      5.50 | `1 KiB writes`
|              171.31 |            5,837.33 |    0.6% |      5.51 | `2 KiB writes`
|              119.96 |            8,336.43 |    0.8% |      5.51 | `4 KiB writes`
|               86.52 |           11,557.81 |    1.2% |      5.52 | `8 KiB writes`
|               80.48 |           12,425.03 |    1.3% |      5.52 | `16 KiB writes`
|               79.27 |           12,614.38 |    1.2% |      5.46 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           18.58 GiB | 1 KiB writes
|           31.63 GiB | 2 KiB writes
|           45.45 GiB | 4 KiB writes
|           62.89 GiB | 8 KiB writes
|           68.23 GiB | 16 KiB writes
|           67.41 GiB | 32 KiB writes
