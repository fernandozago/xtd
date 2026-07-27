Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* CPU governor is 'powersave' but should be 'performance'
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|          ns/message |           message/s |    err% |     total | xtd::channel throughput
|--------------------:|--------------------:|--------:|----------:|:------------------------
|               14.69 |       68,059,654.41 |    0.4% |     13.73 | `single-thread / bounded_channel`
|               14.10 |       70,903,372.21 |    0.4% |     13.74 | `single-thread / unbounded_channel`
|              204.33 |        4,894,154.07 |    1.0% |     13.81 | `multi-thread / bounded_channel`
|              186.82 |        5,352,669.99 |    1.5% |     13.66 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             943,723,622 | `single-thread / bounded_channel`
|             984,874,197 | `single-thread / unbounded_channel`
|              68,194,498 | `multi-thread / bounded_channel`
|              74,103,754 | `multi-thread / unbounded_channel`
