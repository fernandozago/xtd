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
|              263.64 |            3,793.03 |    1.0% |      6.88 | `1 KB writes`
|              158.87 |            6,294.43 |    1.2% |      6.83 | `2 KB writes`
|              113.97 |            8,773.91 |    1.5% |      6.94 | `4 KB writes`
|               81.72 |           12,237.25 |    1.3% |      6.94 | `8 KB writes`
|               78.70 |           12,706.55 |    1.7% |      6.81 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.52 GB | `1 KB writes`
|            42.23 GB | `2 KB writes`
|            60.10 GB | `4 KB writes`
|            83.63 GB | `8 KB writes`
|            86.21 GB | `16 KB writes`
