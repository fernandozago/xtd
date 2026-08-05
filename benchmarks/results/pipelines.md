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
|              289.33 |            3,456.31 |    1.1% |      6.85 | `1 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     6624193
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              170.14 |            5,877.58 |    1.2% |      6.88 | `2 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     10597133
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              122.64 |            8,153.84 |    1.3% |      6.93 | `4 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     14599693
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               87.49 |           11,429.36 |    1.8% |      6.80 | `8 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     20080542
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               85.44 |           11,703.59 |    1.7% |      6.96 | `16 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     21271760
  discarded buffers:  0
  peak active:        32
  peak total:         32

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.12 GB | `1 KB writes`
|            39.73 GB | `2 KB writes`
|            55.69 GB | `4 KB writes`
|            76.60 GB | `8 KB writes`
|            81.15 GB | `16 KB writes`
