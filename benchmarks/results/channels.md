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
|               16.81 |       59,482,069.78 |    2.7% |     14.39 | `single-thread / bounded_channel`
|               15.70 |       63,693,147.72 |    0.5% |     13.45 | `single-thread / unbounded_channel`
|              161.25 |        6,201,705.75 |    1.4% |     13.75 | `multi-thread / bounded_channel`
|              151.60 |        6,596,114.45 |    0.8% |     13.72 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             847,911,122 | `single-thread / bounded_channel`
|             853,941,888 | `single-thread / unbounded_channel`
|              86,453,329 | `multi-thread / bounded_channel`
|              92,094,655 | `multi-thread / unbounded_channel`
