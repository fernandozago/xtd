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
|              314.40 |            3,180.62 |    2.1% |      6.98 | `1 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    33
  retained buffers:   33
  reused buffers:     6256307
  discarded buffers:  0
|              194.84 |            5,132.40 |    1.6% |      6.88 | `2 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    33
  retained buffers:   33
  reused buffers:     9214741
  discarded buffers:  0
|              127.87 |            7,820.53 |    2.3% |      6.84 | `4 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     13923515
  discarded buffers:  0
|               87.73 |           11,398.42 |    0.8% |      6.87 | `8 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     20225006
  discarded buffers:  0
|               82.57 |           12,111.65 |    0.9% |      6.84 | `16 KB writes`
fixed_pool_resource usage:
  max pool size:      33
  created buffers:    32
  retained buffers:   32
  reused buffers:     21592416
  discarded buffers:  0

|   Total Transferred | xtd::pipeline throughput 
|--------------------:|:-------------------------
|            21.98 GB | `1 KB writes`
|            34.57 GB | `2 KB writes`
|            53.11 GB | `4 KB writes`
|            77.15 GB | `8 KB writes`
|            82.37 GB | `16 KB writes`
