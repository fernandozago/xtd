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
|              277.79 |            3,599.81 |    1.6% |      5.50 | `1 KiB writes`
|              163.21 |            6,127.20 |    0.9% |      5.49 | `2 KiB writes`
|              109.89 |            9,100.27 |    1.6% |      5.45 | `4 KiB writes`
|               83.73 |           11,943.20 |    1.3% |      5.51 | `8 KiB writes`
|               77.52 |           12,899.41 |    1.5% |      5.46 | `16 KiB writes`
|               77.60 |           12,887.38 |    1.0% |      5.46 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           19.34 GiB | 1 KiB writes
|           32.94 GiB | 2 KiB writes
|           48.64 GiB | 4 KiB writes
|           65.33 GiB | 8 KiB writes
|           70.46 GiB | 16 KiB writes
|           68.93 GiB | 32 KiB writes
