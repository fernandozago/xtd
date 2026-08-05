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
|              284.42 |            3,515.99 |    1.1% |      6.88 | `1 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     6761667
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              176.23 |            5,674.45 |    1.2% |      6.82 | `2 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     10202966
  discarded buffers:  0
  peak active:        32
  peak total:         32
|              121.06 |            8,260.63 |    1.8% |      6.90 | `4 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     14540490
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               88.47 |           11,303.59 |    0.7% |      6.86 | `8 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     20011784
  discarded buffers:  0
  peak active:        32
  peak total:         32
|               81.86 |           12,215.24 |    0.9% |      6.89 | `16 KB writes`
fixed_pool_resource usage:
  max pool size:      32
  created buffers:    32
  retained buffers:   32
  reused buffers:     21920964
  discarded buffers:  0
  peak active:        32
  peak total:         32

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            23.71 GB | `1 KB writes`
|            38.22 GB | `2 KB writes`
|            55.47 GB | `4 KB writes`
|            76.34 GB | `8 KB writes`
|            83.62 GB | `16 KB writes`
