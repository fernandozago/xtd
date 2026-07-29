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
|               16.07 |       62,210,057.08 |    0.2% |     13.66 | `single-thread / bounded_channel`
|               14.94 |       66,915,734.44 |    0.3% |     13.73 | `single-thread / unbounded_channel`
|              202.22 |        4,945,050.51 |    1.6% |     13.72 | `multi-thread / bounded_channel`
|              190.36 |        5,253,154.88 |    1.0% |     13.71 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             856,902,328 | `single-thread / bounded_channel`
|             929,421,824 | `single-thread / unbounded_channel`
|              68,573,821 | `multi-thread / bounded_channel`
|              73,703,140 | `multi-thread / unbounded_channel`
