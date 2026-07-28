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
|              259.76 |            3,849.73 |    1.1% |      6.94 | `1 KB writes`
|              153.36 |            6,520.49 |    0.9% |      6.87 | `2 KB writes`
|              107.86 |            9,271.38 |    1.1% |      6.88 | `4 KB writes`
|               80.96 |           12,351.33 |    1.2% |      6.88 | `8 KB writes`
|               79.59 |           12,564.67 |    1.1% |      6.88 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            26.40 GB | `1 KB writes`
|            43.97 GB | `2 KB writes`
|            62.83 GB | `4 KB writes`
|            83.78 GB | `8 KB writes`
|            85.70 GB | `16 KB writes`
