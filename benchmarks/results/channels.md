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
|               15.26 |       65,523,000.05 |    0.5% |     13.72 | `single-thread / bounded_channel`
|               15.49 |       64,566,641.45 |    0.2% |     13.74 | `single-thread / unbounded_channel`
|              206.41 |        4,844,638.63 |    2.1% |     13.95 | `multi-thread / bounded_channel`
|              198.22 |        5,044,871.90 |    2.9% |     14.15 | `multi-thread / unbounded_channel`

| Total Items Enqueued | xtd::channel throughput
|---------------------:|:-----------------------
|          910,942,684 | `single-thread / bounded_channel`
|          898,599,494 | `single-thread / unbounded_channel`
|           68,792,581 | `multi-thread / bounded_channel`
|           72,255,847 | `multi-thread / unbounded_channel`
