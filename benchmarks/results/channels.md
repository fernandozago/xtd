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
|               15.33 |       65,232,871.68 |    1.2% |     13.81 | `single-thread / bounded_channel`
|               14.65 |       68,254,795.91 |    0.2% |     13.74 | `single-thread / unbounded_channel`
|              197.52 |        5,062,651.85 |    0.7% |     13.97 | `multi-thread / bounded_channel`
|              186.68 |        5,356,726.65 |    2.3% |     13.88 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             908,810,198 | `single-thread / bounded_channel`
|             947,794,140 | `single-thread / unbounded_channel`
|              72,470,620 | `multi-thread / bounded_channel`
|              76,275,084 | `multi-thread / unbounded_channel`
