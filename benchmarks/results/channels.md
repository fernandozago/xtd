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
|               16.73 |       59,763,359.31 |    0.3% |     13.75 | `single-thread / bounded_channel`
|               14.31 |       69,858,094.42 |    0.4% |     13.70 | `single-thread / unbounded_channel`
|              195.57 |        5,113,254.46 |    2.6% |     13.29 | `multi-thread / bounded_channel`
|              194.19 |        5,149,656.57 |    2.5% |     13.57 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             832,341,663 | `single-thread / bounded_channel`
|             969,287,104 | `single-thread / unbounded_channel`
|              68,149,599 | `multi-thread / bounded_channel`
|              70,269,372 | `multi-thread / unbounded_channel`
