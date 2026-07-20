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
|               14.64 |       68,306,846.56 |    3.1% |     13.50 | `single-thread / bounded_channel`
|               13.48 |       74,189,686.45 |    1.4% |     14.02 | `single-thread / unbounded_channel`
|              182.04 |        5,493,301.21 |    5.7% |     13.63 | :wavy_dash: `multi-thread / bounded_channel` (Unstable with ~2,984,931.2 iters. Increase `minEpochIterations` to e.g. 29849312)
|              195.06 |        5,126,600.87 |    3.3% |     13.55 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             946,295,782 | `single-thread / bounded_channel`
|           1,034,485,356 | `single-thread / unbounded_channel`
|              75,734,391 | `multi-thread / bounded_channel`
|              70,140,986 | `multi-thread / unbounded_channel`
