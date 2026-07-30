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
|               15.59 |       64,133,503.90 |    1.0% |     13.47 | `single-thread / bounded_channel`
|               16.00 |       62,484,159.19 |    2.8% |     15.11 | `single-thread / unbounded_channel`
|              178.16 |        5,612,878.84 |    3.9% |     16.29 | `multi-thread / bounded_channel`
|              176.98 |        5,650,382.49 |    6.7% |     12.99 | :wavy_dash: `multi-thread / unbounded_channel` (Unstable with ~2,356,442.5 iters. Increase `minEpochIterations` to e.g. 23564425)

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             828,440,100 | `single-thread / bounded_channel`
|             857,644,304 | `single-thread / unbounded_channel`
|              80,039,419 | `multi-thread / bounded_channel`
|              60,022,172 | `multi-thread / unbounded_channel`
