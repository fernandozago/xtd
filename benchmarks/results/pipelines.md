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
|              273.63 |            3,654.52 |    0.8% |      6.78 | `1 KB writes`
|              162.44 |            6,156.19 |    1.4% |      6.92 | `2 KB writes`
|              101.10 |            9,891.53 |    0.7% |      6.77 | `4 KB writes`
|               81.61 |           12,252.72 |    1.4% |      6.84 | `8 KB writes`
|               82.57 |           12,111.02 |    1.5% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.54 GB | `1 KB writes`
|            41.90 GB | `2 KB writes`
|            65.40 GB | `4 KB writes`
|            82.77 GB | `8 KB writes`
|            83.18 GB | `16 KB writes`
