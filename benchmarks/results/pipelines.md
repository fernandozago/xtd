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
|              269.53 |            3,710.17 |    1.4% |      6.90 | `1 KB writes`
|              162.54 |            6,152.51 |    2.6% |      7.00 | `2 KB writes`
|              115.10 |            8,687.78 |    0.8% |      6.86 | `4 KB writes`
|               82.90 |           12,063.25 |    1.0% |      6.86 | `8 KB writes`
|               76.03 |           13,153.38 |    2.3% |      6.92 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.37 GB | `1 KB writes`
|            42.40 GB | `2 KB writes`
|            58.65 GB | `4 KB writes`
|            81.59 GB | `8 KB writes`
|            89.89 GB | `16 KB writes`
