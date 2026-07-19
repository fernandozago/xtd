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
|              172.15 |            5,808.78 |    2.0% |      5.32 | `1 KiB writes`
|              123.87 |            8,072.89 |    1.0% |      5.44 | `2 KiB writes`
|               89.31 |           11,197.28 |    1.9% |      5.44 | `4 KiB writes`
|               78.66 |           12,712.45 |    1.2% |      5.49 | `8 KiB writes`
|               79.77 |           12,535.53 |    1.1% |      5.54 | `16 KiB writes`
|               80.13 |           12,479.53 |    2.2% |      5.53 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           30.14 GiB | 1 KiB writes
|           43.34 GiB | 2 KiB writes
|           59.77 GiB | 4 KiB writes
|           69.28 GiB | 8 KiB writes
|           70.01 GiB | 16 KiB writes
|           67.10 GiB | 32 KiB writes
