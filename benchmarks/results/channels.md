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
|               15.56 |       64,272,002.73 |    0.5% |     13.82 | `single-thread / bounded_channel`
|               15.86 |       63,056,166.32 |    0.4% |     13.76 | `single-thread / unbounded_channel`
|              204.52 |        4,889,398.44 |    0.8% |     14.18 | `multi-thread / bounded_channel`
|              199.70 |        5,007,592.70 |    1.1% |     13.81 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             896,730,846 | `single-thread / bounded_channel`
|             878,889,788 | `single-thread / unbounded_channel`
|              70,597,198 | `multi-thread / bounded_channel`
|              70,093,614 | `multi-thread / unbounded_channel`
