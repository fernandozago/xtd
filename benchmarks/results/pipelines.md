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
|              264.68 |            3,778.10 |    1.4% |      6.90 | `1 KB writes`
|              155.95 |            6,412.27 |    1.2% |      6.77 | `2 KB writes`
|              108.32 |            9,231.69 |    1.4% |      6.93 | `4 KB writes`
|               84.02 |           11,901.54 |    0.9% |      6.87 | `8 KB writes`
|               77.06 |           12,976.66 |    0.9% |      6.84 | `16 KB writes`

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            25.64 GB | `1 KB writes`
|            42.87 GB | `2 KB writes`
|            62.93 GB | `4 KB writes`
|            81.03 GB | `8 KB writes`
|            87.88 GB | `16 KB writes`
