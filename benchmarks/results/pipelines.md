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
|              264.54 |            3,780.11 |    1.5% |      6.85 | `1 KB writes`
|              165.86 |            6,029.26 |    1.8% |      6.80 | `2 KB writes`
|              108.62 |            9,206.80 |    2.3% |      6.96 | `4 KB writes`
|               85.56 |           11,687.08 |    1.0% |      6.87 | `8 KB writes`
|               80.58 |           12,410.46 |    2.4% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.33 GB | `1 KB writes`
|            40.33 GB | `2 KB writes`
|            62.99 GB | `4 KB writes`
|            78.75 GB | `8 KB writes`
|            85.39 GB | `16 KB writes`
