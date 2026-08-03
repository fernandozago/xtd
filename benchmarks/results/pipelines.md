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
|              304.04 |            3,289.06 |    0.4% |      6.88 | `1 KB writes`
|              189.21 |            5,285.20 |    1.0% |      6.88 | `2 KB writes`
|              124.99 |            8,000.39 |    0.9% |      6.81 | `4 KB writes`
|               87.82 |           11,386.88 |    1.9% |      6.89 | `8 KB writes`
|               81.39 |           12,286.24 |    0.8% |      6.90 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            22.24 GB | `1 KB writes`
|            35.71 GB | `2 KB writes`
|            53.34 GB | `4 KB writes`
|            77.80 GB | `8 KB writes`
|            84.14 GB | `16 KB writes`
