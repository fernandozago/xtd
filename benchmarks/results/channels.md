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
|               14.83 |       67,420,050.62 |    0.4% |     13.79 | `single-thread / bounded_channel`
|               14.33 |       69,804,314.50 |    0.3% |     13.77 | `single-thread / unbounded_channel`
|              201.08 |        4,973,246.17 |    2.0% |     13.79 | `multi-thread / bounded_channel`
|              187.45 |        5,334,671.65 |    2.6% |     13.90 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             939,699,625 | `single-thread / bounded_channel`
|             972,129,420 | `single-thread / unbounded_channel`
|              69,942,229 | `multi-thread / bounded_channel`
|              77,407,880 | `multi-thread / unbounded_channel`
