Machine Spec:
  CPU model: Intel(R) Core(TM) i5-1035G7 CPU @ 1.20GHz
  CPU cores (physical/logical per socket): 4 / 8
  Memory: 14.4625 GiB total

Warning, results might be unstable:
* CPU frequency scaling enabled: CPU 0 between 400.0 and 3,700.0 MHz
* Turbo is enabled, CPU frequency will fluctuate

Recommendations
* Use 'pyperf system tune' before benchmarking. See https://github.com/psf/pyperf

|             ns/item |              item/s |    err% |     total | xtd::channel throughput
|--------------------:|--------------------:|--------:|----------:|:------------------------
|               15.24 |       65,606,119.91 |    0.2% |     13.75 | `single-thread / bounded_channel`
|               15.47 |       64,637,069.49 |    0.3% |     13.75 | `single-thread / unbounded_channel`
|              211.12 |        4,736,602.97 |    3.9% |     14.09 | `multi-thread / bounded_channel`
|              189.23 |        5,284,541.21 |    2.4% |     13.61 | `multi-thread / unbounded_channel`

| Total Items Enqueued | xtd::channel throughput
|---------------------:|:-----------------------
|          912,393,204 | `single-thread / bounded_channel`
|          899,854,659 | `single-thread / unbounded_channel`
|           67,350,223 | `multi-thread / bounded_channel`
|           72,178,554 | `multi-thread / unbounded_channel`
