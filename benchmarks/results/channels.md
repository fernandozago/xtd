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
|               22.49 |       44,462,322.25 |    0.6% |      6.86 | `single-thread / bounded_channel`
|               23.67 |       42,252,441.62 |    0.4% |      6.81 | `single-thread / unbounded_channel`
|              164.01 |        6,097,208.78 |    4.7% |      6.86 | `multi-thread / bounded_channel`
|              164.44 |        6,081,281.55 |    0.4% |      6.67 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             305,853,949 | `single-thread / bounded_channel`
|             288,643,518 | `single-thread / unbounded_channel`
|              43,144,596 | `multi-thread / bounded_channel`
|              40,340,830 | `multi-thread / unbounded_channel`
