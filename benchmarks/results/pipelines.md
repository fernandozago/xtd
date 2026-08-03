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
|              289.04 |            3,459.69 |    0.6% |      6.85 | `1 KB writes`
|              177.07 |            5,647.35 |    1.8% |      6.83 | `2 KB writes`
|              122.30 |            8,176.28 |    0.9% |      6.85 | `4 KB writes`
|               84.49 |           11,836.12 |    2.2% |      7.00 | `8 KB writes`
|               81.18 |           12,317.86 |    1.9% |      6.91 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.23 GB | `1 KB writes`
|            37.76 GB | `2 KB writes`
|            55.35 GB | `4 KB writes`
|            81.48 GB | `8 KB writes`
|            84.16 GB | `16 KB writes`
