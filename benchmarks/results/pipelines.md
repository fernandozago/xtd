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
|              267.72 |            3,735.25 |    0.8% |      6.90 | `1 KB writes`
|              161.29 |            6,200.11 |    3.4% |      6.80 | `2 KB writes`
|              116.70 |            8,569.32 |    1.1% |      6.93 | `4 KB writes`
|               84.12 |           11,887.34 |    2.0% |      6.80 | `8 KB writes`
|               81.93 |           12,204.89 |    1.0% |      6.89 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.31 GB | `1 KB writes`
|            42.24 GB | `2 KB writes`
|            58.48 GB | `4 KB writes`
|            79.80 GB | `8 KB writes`
|            83.67 GB | `16 KB writes`
