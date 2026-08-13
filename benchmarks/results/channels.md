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
|               14.71 |       67,968,158.55 |    0.4% |     13.74 | `single-thread / bounded_channel`
|               14.95 |       66,894,273.30 |    0.2% |     13.75 | `single-thread / unbounded_channel`
|              207.36 |        4,822,445.61 |    1.4% |     13.83 | `multi-thread / bounded_channel`
|              193.84 |        5,158,884.42 |    1.6% |     13.63 | `multi-thread / unbounded_channel`

| Total Items Enqueued | xtd::channel throughput
|---------------------:|:-----------------------
|          942,436,340 | `single-thread / bounded_channel`
|          929,973,102 | `single-thread / unbounded_channel`
|           67,867,636 | `multi-thread / bounded_channel`
|           71,219,168 | `multi-thread / unbounded_channel`
