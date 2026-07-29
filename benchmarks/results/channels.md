Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|          ns/message |           message/s |    err% |     total | xtd::channel throughput
|--------------------:|--------------------:|--------:|----------:|:------------------------
|               14.51 |       68,941,737.75 |    0.8% |     13.84 | `single-thread / bounded_channel`
|               13.75 |       72,708,896.17 |    1.0% |     13.77 | `single-thread / unbounded_channel`
|              196.90 |        5,078,725.27 |    0.9% |     13.90 | `multi-thread / bounded_channel`
|              186.52 |        5,361,328.46 |    1.5% |     13.68 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             956,441,740 | `single-thread / bounded_channel`
|           1,010,218,028 | `single-thread / unbounded_channel`
|              71,616,305 | `multi-thread / bounded_channel`
|              74,668,060 | `multi-thread / unbounded_channel`
