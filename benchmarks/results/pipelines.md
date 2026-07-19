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
|              217.07 |            4,606.76 |    0.7% |      5.47 | `1 KiB writes`
|              134.63 |            7,427.65 |    1.2% |      5.44 | `2 KiB writes`
|               95.66 |           10,453.71 |    1.8% |      5.43 | `4 KiB writes`
|               74.38 |           13,444.62 |    2.6% |      5.47 | `8 KiB writes`
|               68.78 |           14,539.71 |    2.2% |      5.40 | `16 KiB writes`
|               76.79 |           13,023.15 |    0.8% |      5.49 | `32 KiB writes`

|    Total Transfered | xtd::pipeline throughput 
|--------------------:|:-------------------------
|           24.52 GiB | 1 KiB writes
|           39.66 GiB | 2 KiB writes
|           55.32 GiB | 4 KiB writes
|           72.39 GiB | 8 KiB writes
|           77.22 GiB | 16 KiB writes
|           69.29 GiB | 32 KiB writes
