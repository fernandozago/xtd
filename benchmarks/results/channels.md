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
|               16.02 |       62,412,311.62 |    2.1% |     13.53 | `single-thread / bounded_channel`
|               13.74 |       72,762,694.66 |    0.2% |     13.80 | `single-thread / unbounded_channel`
|              202.03 |        4,949,641.56 |    1.3% |     13.85 | `multi-thread / bounded_channel`
|              202.01 |        4,950,147.66 |    1.4% |     13.76 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             853,435,016 | `single-thread / bounded_channel`
|           1,010,230,029 | `single-thread / unbounded_channel`
|              69,981,053 | `multi-thread / bounded_channel`
|              69,270,849 | `multi-thread / unbounded_channel`
