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
|              279.38 |            3,579.40 |    0.9% |      6.86 | `1 KB writes`
|              175.00 |            5,714.43 |    1.8% |      6.95 | `2 KB writes`
|              123.94 |            8,068.60 |    1.2% |      6.94 | `4 KB writes`
|               88.18 |           11,340.81 |    1.3% |      6.87 | `8 KB writes`
|               84.60 |           11,820.19 |    0.9% |      6.87 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.95 GB | `1 KB writes`
|            38.88 GB | `2 KB writes`
|            55.09 GB | `4 KB writes`
|            76.58 GB | `8 KB writes`
|            80.76 GB | `16 KB writes`
