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
|               16.60 |       60,228,465.25 |    1.3% |     13.59 | `single-thread / bounded_channel`
|               15.50 |       64,503,452.41 |    1.4% |     13.57 | `single-thread / unbounded_channel`
|              198.63 |        5,034,565.77 |    3.6% |     13.76 | `multi-thread / bounded_channel`
|              175.77 |        5,689,112.64 |    1.9% |     13.63 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             831,438,850 | `single-thread / bounded_channel`
|             890,055,912 | `single-thread / unbounded_channel`
|              70,920,258 | `multi-thread / bounded_channel`
|              78,144,502 | `multi-thread / unbounded_channel`
