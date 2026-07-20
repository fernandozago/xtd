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
|               16.04 |       62,347,314.28 |    0.6% |     13.70 | `single-thread / bounded_channel`
|               14.64 |       68,311,628.46 |    0.3% |     13.79 | `single-thread / unbounded_channel`
|              197.76 |        5,056,716.25 |    2.5% |     13.52 | `multi-thread / bounded_channel`
|              192.26 |        5,201,314.04 |    3.6% |     13.84 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             867,170,388 | `single-thread / bounded_channel`
|             952,197,587 | `single-thread / unbounded_channel`
|              68,854,133 | `multi-thread / bounded_channel`
|              73,791,643 | `multi-thread / unbounded_channel`
