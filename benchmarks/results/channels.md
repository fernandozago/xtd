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
|               14.69 |       68,088,609.69 |    0.5% |     13.71 | `single-thread / bounded_channel`
|               14.24 |       70,246,236.25 |    0.3% |     13.78 | `single-thread / unbounded_channel`
|              201.10 |        4,972,712.26 |    2.3% |     13.76 | `multi-thread / bounded_channel`
|              183.98 |        5,435,509.30 |    2.5% |     13.74 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             944,098,027 | `single-thread / bounded_channel`
|             979,795,774 | `single-thread / unbounded_channel`
|              69,317,684 | `multi-thread / bounded_channel`
|              74,630,953 | `multi-thread / unbounded_channel`
