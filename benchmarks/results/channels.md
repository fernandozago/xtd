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
|               20.44 |       48,923,157.08 |    0.6% |      6.80 | `single-thread / bounded_channel`
|               18.65 |       53,605,709.57 |    1.0% |      6.80 | `single-thread / unbounded_channel`
|              156.79 |        6,377,820.56 |    0.5% |      6.91 | `multi-thread / bounded_channel`
|              161.40 |        6,195,823.66 |    1.2% |      6.63 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             332,271,920 | `single-thread / bounded_channel`
|             364,446,103 | `single-thread / unbounded_channel`
|              45,285,662 | `multi-thread / bounded_channel`
|              40,712,976 | `multi-thread / unbounded_channel`
