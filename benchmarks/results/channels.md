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
|               14.57 |       68,632,365.13 |    0.4% |     13.89 | `single-thread / bounded_channel`
|               15.55 |       64,309,680.60 |    0.2% |     13.76 | `single-thread / unbounded_channel`
|              207.76 |        4,813,226.61 |    2.3% |     13.42 | `multi-thread / bounded_channel`
|              194.25 |        5,148,075.44 |    0.9% |     13.98 | `multi-thread / unbounded_channel`

| Total Items Enqueued | xtd::channel throughput
|---------------------:|:-----------------------
|          958,789,657 | `single-thread / bounded_channel`
|          896,591,924 | `single-thread / unbounded_channel`
|           66,038,780 | `multi-thread / bounded_channel`
|           73,284,575 | `multi-thread / unbounded_channel`
