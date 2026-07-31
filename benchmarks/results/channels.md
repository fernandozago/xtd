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
|               15.45 |       64,736,783.47 |    0.4% |     13.71 | `single-thread / bounded_channel`
|               15.50 |       64,521,970.87 |    0.2% |     13.76 | `single-thread / unbounded_channel`
|              192.68 |        5,190,084.50 |    2.2% |     13.75 | `multi-thread / bounded_channel`
|              185.81 |        5,381,755.75 |    3.5% |     13.86 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             894,506,808 | `single-thread / bounded_channel`
|             898,505,035 | `single-thread / unbounded_channel`
|              72,012,863 | `multi-thread / bounded_channel`
|              76,304,716 | `multi-thread / unbounded_channel`
