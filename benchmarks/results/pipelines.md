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
|              266.01 |            3,759.30 |    1.0% |      5.51 | `1 KiB writes`
|              156.94 |            6,371.76 |    2.1% |      5.45 | `2 KiB writes`
|              106.81 |            9,362.48 |    1.1% |      5.48 | `4 KiB writes`
|               80.34 |           12,447.64 |    1.3% |      5.45 | `8 KiB writes`
|               80.60 |           12,407.53 |    1.1% |      5.44 | `16 KiB writes`
|               78.70 |           12,706.82 |    0.7% |      5.43 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           20.34 GiB | 1 KiB writes
|           34.26 GiB | 2 KiB writes
|           50.58 GiB | 4 KiB writes
|           66.49 GiB | 8 KiB writes
|           67.34 GiB | 16 KiB writes
|           67.48 GiB | 32 KiB writes
