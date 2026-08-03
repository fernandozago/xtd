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
|               15.10 |       66,218,090.96 |    0.4% |     13.80 | `single-thread / bounded_channel`
|               15.46 |       64,703,002.27 |    0.5% |     13.81 | `single-thread / unbounded_channel`
|              227.22 |        4,400,949.39 |   16.3% |     14.80 | :wavy_dash: `multi-thread / bounded_channel` (Unstable with ~1,647,679.3 iters. Increase `minEpochIterations` to e.g. 16476793)
|              181.53 |        5,508,724.40 |    7.0% |     14.03 | :wavy_dash: `multi-thread / unbounded_channel` (Unstable with ~3,078,972.6 iters. Increase `minEpochIterations` to e.g. 30789726)

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             921,073,299 | `single-thread / bounded_channel`
|             903,350,241 | `single-thread / unbounded_channel`
|              42,303,092 | `multi-thread / bounded_channel`
|              78,085,424 | `multi-thread / unbounded_channel`
