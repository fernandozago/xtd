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
|              299.02 |            3,344.27 |    1.3% |      6.90 | `1 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     6445628
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              178.21 |            5,611.36 |    1.2% |      6.91 | `2 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     10180509
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              124.43 |            8,036.58 |    2.9% |      7.00 | `4 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     14470978
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               96.50 |           10,362.64 |    1.9% |      7.01 | `8 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     18787572
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               92.70 |           10,787.42 |    4.9% |      7.19 | `16 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     19655244
  discarded buffers:  0
  peak active:        32
  peak total:         32

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            22.69 GB | `1 KB writes`
|            38.05 GB | `2 KB writes`
|            55.20 GB | `4 KB writes`
|            71.67 GB | `8 KB writes`
|            74.98 GB | `16 KB writes`
