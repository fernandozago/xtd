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
|               17.46 |       57,274,126.39 |    0.8% |     13.56 | `single-thread / bounded_channel`
|               16.06 |       62,284,323.84 |    0.6% |     13.67 | `single-thread / unbounded_channel`
|              162.37 |        6,158,867.80 |    1.9% |     13.66 | `multi-thread / bounded_channel`
|              151.90 |        6,583,174.28 |    6.3% |     14.13 | :wavy_dash: `multi-thread / unbounded_channel` (Unstable with ~3,700,304.9 iters. Increase `minEpochIterations` to e.g. 37003049)

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             784,227,074 | `single-thread / bounded_channel`
|             862,322,462 | `single-thread / unbounded_channel`
|              84,926,302 | `multi-thread / bounded_channel`
|              93,618,733 | `multi-thread / unbounded_channel`
