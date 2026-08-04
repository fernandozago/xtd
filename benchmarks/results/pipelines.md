Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|              μs/MB |                MB/s |    err% |     total | xtd::pipeline throughput
|--------------------:|--------------------:|--------:|----------:|:-------------------------
|              279.53 |            3,577.44 |    1.8% |      6.74 | `1 KB writes`
|              170.80 |            5,854.79 |    2.0% |      6.92 | `2 KB writes`
|              115.30 |            8,672.77 |    1.3% |      6.80 | `4 KB writes`
|               83.38 |           11,993.00 |    1.3% |      6.82 | `8 KB writes`
|               81.79 |           12,226.28 |    0.8% |      6.92 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.49 GB | `1 KB writes`
|            39.85 GB | `2 KB writes`
|            58.42 GB | `4 KB writes`
|            80.27 GB | `8 KB writes`
|            84.60 GB | `16 KB writes`
