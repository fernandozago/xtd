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
|              279.12 |            3,582.74 |    1.3% |      6.89 | `1 KB writes`
|              164.67 |            6,072.74 |    2.4% |      6.76 | `2 KB writes`
|              109.47 |            9,135.19 |    2.3% |      6.84 | `4 KB writes`
|               83.69 |           11,948.46 |    4.0% |      6.92 | `8 KB writes`
|               81.77 |           12,229.64 |    1.8% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.21 GB | `1 KB writes`
|            40.48 GB | `2 KB writes`
|            61.79 GB | `4 KB writes`
|            81.91 GB | `8 KB writes`
|            83.74 GB | `16 KB writes`
