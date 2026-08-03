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
|               14.45 |       69,207,984.39 |    0.4% |     13.74 | `single-thread / bounded_channel`
|               14.76 |       67,744,654.45 |    0.4% |     13.77 | `single-thread / unbounded_channel`
|              196.00 |        5,101,939.78 |    5.5% |     13.58 | :wavy_dash: `multi-thread / bounded_channel` (Unstable with ~2,743,760.9 iters. Increase `minEpochIterations` to e.g. 27437609)
|              174.02 |        5,746,326.72 |    2.0% |     13.73 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             960,213,103 | `single-thread / bounded_channel`
|             944,251,908 | `single-thread / unbounded_channel`
|              69,705,132 | `multi-thread / bounded_channel`
|              79,693,283 | `multi-thread / unbounded_channel`
