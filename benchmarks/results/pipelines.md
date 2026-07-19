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
|              225.69 |            4,430.85 |    1.0% |      5.47 | `1 KiB writes`
|              137.25 |            7,286.07 |    0.8% |      5.49 | `2 KiB writes`
|               93.83 |           10,657.01 |    0.6% |      5.53 | `4 KiB writes`
|               77.77 |           12,858.64 |    0.4% |      5.53 | `8 KiB writes`
|               72.17 |           13,856.20 |    0.5% |      5.46 | `16 KiB writes`
|               80.85 |           12,368.04 |    0.8% |      5.49 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           23.83 GiB | 1 KiB writes
|           39.10 GiB | 2 KiB writes
|           57.89 GiB | 4 KiB writes
|           70.31 GiB | 8 KiB writes
|           75.07 GiB | 16 KiB writes
|           66.01 GiB | 32 KiB writes
