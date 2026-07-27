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
|              268.15 |            3,729.32 |    2.0% |      6.93 | `1 KB writes`
|              159.75 |            6,259.82 |    3.3% |      6.76 | `2 KB writes`
|              114.51 |            8,733.20 |    0.6% |      6.88 | `4 KB writes`
|               85.71 |           11,667.57 |    1.2% |      6.96 | `8 KB writes`
|               80.88 |           12,363.67 |    2.2% |      6.91 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.44 GB | `1 KB writes`
|            41.88 GB | `2 KB writes`
|            58.90 GB | `4 KB writes`
|            80.77 GB | `8 KB writes`
|            85.67 GB | `16 KB writes`
