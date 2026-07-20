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
|               15.86 |       63,061,385.45 |    1.2% |     13.86 | `single-thread / bounded_channel`
|               13.82 |       72,345,738.60 |    0.5% |     13.71 | `single-thread / unbounded_channel`
|              201.85 |        4,954,171.00 |    0.8% |     13.86 | `multi-thread / bounded_channel`
|              205.87 |        4,857,489.11 |    1.4% |     13.90 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             883,998,719 | `single-thread / bounded_channel`
|           1,003,607,671 | `single-thread / unbounded_channel`
|              69,171,803 | `multi-thread / bounded_channel`
|              69,174,771 | `multi-thread / unbounded_channel`
