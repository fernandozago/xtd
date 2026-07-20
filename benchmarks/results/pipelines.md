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
|              266.52 |            3,752.13 |    1.9% |      5.50 | `1 KiB writes`
|              159.50 |            6,269.66 |    1.9% |      5.59 | `2 KiB writes`
|              112.49 |            8,889.71 |    0.4% |      5.49 | `4 KiB writes`
|               83.15 |           12,027.12 |    0.8% |      5.52 | `8 KiB writes`
|               81.06 |           12,336.51 |    1.3% |      5.46 | `16 KiB writes`
|               81.84 |           12,219.05 |    1.4% |      5.53 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           20.31 GiB | 1 KiB writes
|           34.60 GiB | 2 KiB writes
|           48.09 GiB | 4 KiB writes
|           65.41 GiB | 8 KiB writes
|           67.62 GiB | 16 KiB writes
|           66.09 GiB | 32 KiB writes
