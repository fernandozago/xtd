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
|              277.02 |            3,609.85 |    0.6% |      5.46 | `1 KiB writes`
|              164.67 |            6,072.67 |    0.9% |      5.48 | `2 KiB writes`
|              102.36 |            9,769.15 |    0.9% |      5.41 | `4 KiB writes`
|               80.86 |           12,366.87 |    2.1% |      5.53 | `8 KiB writes`
|               81.63 |           12,249.68 |    1.8% |      5.42 | `16 KiB writes`
|               83.42 |           11,988.24 |    1.6% |      5.40 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           19.32 GiB | 1 KiB writes
|           32.78 GiB | 2 KiB writes
|           51.53 GiB | 4 KiB writes
|           67.82 GiB | 8 KiB writes
|           66.50 GiB | 16 KiB writes
|           63.06 GiB | 32 KiB writes
