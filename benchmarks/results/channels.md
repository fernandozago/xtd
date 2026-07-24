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
|               16.69 |       59,900,760.65 |    0.3% |     13.77 | `single-thread / bounded_channel`
|               14.43 |       69,282,343.06 |    0.2% |     13.77 | `single-thread / unbounded_channel`
|              185.95 |        5,377,798.58 |    3.0% |     13.21 | `multi-thread / bounded_channel`
|              208.41 |        4,798,339.99 |    2.3% |     13.38 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             835,497,073 | `single-thread / bounded_channel`
|             966,306,156 | `single-thread / unbounded_channel`
|              70,405,028 | `multi-thread / bounded_channel`
|              67,313,389 | `multi-thread / unbounded_channel`
