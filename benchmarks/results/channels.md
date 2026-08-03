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
|               16.54 |       60,450,157.85 |    0.7% |     13.72 | `single-thread / bounded_channel`
|               16.51 |       60,585,861.34 |    0.6% |     13.57 | `single-thread / unbounded_channel`
|              179.21 |        5,580,108.73 |    3.0% |     13.37 | `multi-thread / bounded_channel`
|              176.92 |        5,652,430.37 |    5.5% |     14.50 | :wavy_dash: `multi-thread / unbounded_channel` (Unstable with ~3,270,610.4 iters. Increase `minEpochIterations` to e.g. 32706104)

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             833,008,315 | `single-thread / bounded_channel`
|             831,240,569 | `single-thread / unbounded_channel`
|              74,711,047 | `multi-thread / bounded_channel`
|              82,876,370 | `multi-thread / unbounded_channel`
