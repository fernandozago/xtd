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
|               17.12 |       58,399,031.21 |    0.2% |     13.79 | `single-thread / bounded_channel`
|               17.35 |       57,627,413.02 |    0.2% |     13.75 | `single-thread / unbounded_channel`
|              179.86 |        5,559,776.47 |    2.1% |     13.92 | `multi-thread / bounded_channel`
|              160.28 |        6,239,177.06 |    0.6% |     13.78 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             816,327,474 | `single-thread / bounded_channel`
|             803,656,381 | `single-thread / unbounded_channel`
|              79,408,243 | `multi-thread / bounded_channel`
|              87,018,937 | `multi-thread / unbounded_channel`
