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
|              208.05 |            4,806.44 |    1.1% |      5.48 | `1 KiB writes`
|              128.38 |            7,789.53 |    1.1% |      5.48 | `2 KiB writes`
|               88.46 |           11,304.90 |    1.1% |      5.45 | `4 KiB writes`
|               74.20 |           13,477.98 |    1.3% |      5.49 | `8 KiB writes`
|               67.44 |           14,827.82 |    1.0% |      5.46 | `16 KiB writes`
|               75.94 |           13,169.07 |    1.8% |      5.43 | `32 KiB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           25.75 GiB | 1 KiB writes
|           41.86 GiB | 2 KiB writes
|           60.41 GiB | 4 KiB writes
|           73.59 GiB | 8 KiB writes
|           80.20 GiB | 16 KiB writes
|           69.01 GiB | 32 KiB writes
