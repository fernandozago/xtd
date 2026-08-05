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
|              281.49 |            3,552.52 |    1.7% |      6.83 | `1 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    33
  retained buffers:   33
  reused buffers:     6808814
  discarded buffers:  0
|              175.31 |            5,704.15 |    0.9% |      6.83 | `2 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    33
  retained buffers:   33
  reused buffers:     10209394
  discarded buffers:  0
|              108.97 |            9,176.51 |    1.6% |      6.83 | `4 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     16102498
  discarded buffers:  0
|               86.95 |           11,500.95 |    1.7% |      6.98 | `8 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     20778612
  discarded buffers:  0
|               81.73 |           12,234.96 |    0.9% |      6.89 | `16 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     21970724
  discarded buffers:  0

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            24.08 GB | `1 KB writes`
|            38.25 GB | `2 KB writes`
|            61.43 GB | `4 KB writes`
|            79.26 GB | `8 KB writes`
|            83.81 GB | `16 KB writes`
