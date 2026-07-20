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
|              248.14 |            4,029.90 |    1.6% |      5.34 | `1 KiB writes`
|              161.09 |            6,207.60 |    0.8% |      5.55 | `2 KiB writes`
|              107.55 |            9,298.37 |    1.2% |      5.48 | `4 KiB writes`
|               83.47 |           11,980.32 |    0.9% |      5.51 | `8 KiB writes`
|               81.19 |           12,316.61 |    1.0% |      5.49 | `16 KiB writes`
|               79.46 |           12,584.82 |    1.2% |      5.53 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           20.67 GiB | 1 KiB writes
|           34.16 GiB | 2 KiB writes
|           50.16 GiB | 4 KiB writes
|           65.85 GiB | 8 KiB writes
|           67.56 GiB | 16 KiB writes
|           67.82 GiB | 32 KiB writes
