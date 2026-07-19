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
|               14.40 |       69,430,657.88 |    1.0% |     13.79 | `single-thread / bounded_channel`
|               14.70 |       68,043,881.79 |    0.2% |     13.76 | `single-thread / unbounded_channel`
|              171.11 |        5,844,102.66 |    1.2% |     13.60 | `multi-thread / bounded_channel`
|              145.12 |        6,891,052.45 |    2.0% |     13.70 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             961,408,632 | `single-thread / bounded_channel`
|             946,524,272 | `single-thread / unbounded_channel`
|              82,341,249 | `multi-thread / bounded_channel`
|              95,132,717 | `multi-thread / unbounded_channel`
