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
|               15.75 |       63,486,314.75 |    2.1% |     14.14 | `single-thread / bounded_channel`
|               15.58 |       64,168,885.67 |    0.2% |     13.80 | `single-thread / unbounded_channel`
|              195.81 |        5,107,072.02 |    0.7% |     13.64 | `multi-thread / bounded_channel`
|              185.76 |        5,383,214.75 |    1.1% |     13.77 | `multi-thread / unbounded_channel`

| Total Items Enqueued | xtd::channel throughput
|---------------------:|:-----------------------
|          926,709,950 | `single-thread / bounded_channel`
|          897,870,268 | `single-thread / unbounded_channel`
|           70,821,044 | `multi-thread / bounded_channel`
|           74,719,113 | `multi-thread / unbounded_channel`
