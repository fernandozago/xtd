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
|               16.11 |       62,068,864.92 |    0.4% |     13.83 | `single-thread / bounded_channel`
|               15.39 |       64,979,622.20 |    0.2% |     13.76 | `single-thread / unbounded_channel`
|              160.41 |        6,233,993.36 |    1.0% |     13.67 | `multi-thread / bounded_channel`
|              152.32 |        6,565,142.06 |    1.4% |     13.87 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             871,108,766 | `single-thread / bounded_channel`
|             904,622,710 | `single-thread / unbounded_channel`
|              87,050,703 | `multi-thread / bounded_channel`
|              92,498,005 | `multi-thread / unbounded_channel`
