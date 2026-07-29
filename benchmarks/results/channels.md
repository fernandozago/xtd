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
|               15.11 |       66,185,053.99 |    0.5% |     13.68 | `single-thread / bounded_channel`
|               14.36 |       69,615,952.63 |    0.4% |     13.76 | `single-thread / unbounded_channel`
|              194.84 |        5,132,328.02 |    1.4% |     13.66 | `multi-thread / bounded_channel`
|              180.34 |        5,544,991.73 |    1.6% |     13.95 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             915,065,597 | `single-thread / bounded_channel`
|             968,659,654 | `single-thread / unbounded_channel`
|              71,056,484 | `multi-thread / bounded_channel`
|              77,772,624 | `multi-thread / unbounded_channel`
