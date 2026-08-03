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
|               13.87 |       72,087,164.09 |    1.1% |     14.01 | `single-thread / bounded_channel`
|               15.58 |       64,201,829.20 |    1.4% |     13.83 | `single-thread / unbounded_channel`
|              210.77 |        4,744,470.74 |    2.6% |     13.65 | `multi-thread / bounded_channel`
|              191.76 |        5,214,874.68 |    2.1% |     14.02 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|           1,002,759,601 | `single-thread / bounded_channel`
|             895,933,921 | `single-thread / unbounded_channel`
|              67,183,156 | `multi-thread / bounded_channel`
|              74,073,940 | `multi-thread / unbounded_channel`
