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
|               13.72 |       72,890,272.54 |    1.0% |     13.66 | `single-thread / bounded_channel`
|               13.90 |       71,927,118.92 |    0.7% |     13.72 | `single-thread / unbounded_channel`
|              200.63 |        4,984,332.99 |    4.6% |     13.25 | `multi-thread / bounded_channel`
|              198.87 |        5,028,514.06 |    3.9% |     13.91 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|           1,005,383,715 | `single-thread / bounded_channel`
|             993,407,574 | `single-thread / unbounded_channel`
|              66,566,245 | `multi-thread / bounded_channel`
|              70,481,128 | `multi-thread / unbounded_channel`
