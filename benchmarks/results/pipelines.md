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
|              283.26 |            3,530.31 |    1.3% |      5.55 | `1 KiB writes`
|              154.93 |            6,454.49 |    1.9% |      5.57 | `2 KiB writes`
|              105.84 |            9,448.12 |    1.0% |      5.46 | `4 KiB writes`
|               83.64 |           11,955.71 |    1.9% |      5.56 | `8 KiB writes`
|               80.58 |           12,409.60 |    2.3% |      5.49 | `16 KiB writes`
|               82.98 |           12,050.73 |    1.2% |      5.50 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           19.40 GiB | 1 KiB writes
|           35.41 GiB | 2 KiB writes
|           51.01 GiB | 4 KiB writes
|           66.22 GiB | 8 KiB writes
|           68.04 GiB | 16 KiB writes
|           64.80 GiB | 32 KiB writes
