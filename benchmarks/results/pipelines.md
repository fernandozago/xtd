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
|              270.62 |            3,695.16 |    1.0% |      6.89 | `1 KB writes`
|              160.82 |            6,218.11 |    1.2% |      6.88 | `2 KB writes`
|              108.44 |            9,221.88 |    0.7% |      6.84 | `4 KB writes`
|               83.46 |           11,982.16 |    1.9% |      6.94 | `8 KB writes`
|               84.13 |           11,887.03 |    1.0% |      6.83 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.84 GB | `1 KB writes`
|            42.03 GB | `2 KB writes`
|            62.11 GB | `4 KB writes`
|            81.82 GB | `8 KB writes`
|            81.33 GB | `16 KB writes`
