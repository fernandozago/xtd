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
|               14.20 |       70,437,379.00 |    0.7% |     13.67 | `single-thread / bounded_channel`
|               13.70 |       72,968,546.04 |    0.8% |     13.77 | `single-thread / unbounded_channel`
|              187.86 |        5,322,994.25 |    1.0% |     13.51 | `multi-thread / bounded_channel`
|              170.35 |        5,870,411.78 |    1.5% |     13.84 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             971,244,522 | `single-thread / bounded_channel`
|           1,011,954,769 | `single-thread / unbounded_channel`
|              71,896,377 | `multi-thread / bounded_channel`
|              82,484,505 | `multi-thread / unbounded_channel`
