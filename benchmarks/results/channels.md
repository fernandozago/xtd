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
|               15.11 |       66,191,141.92 |    0.2% |     13.78 | `single-thread / bounded_channel`
|               15.53 |       64,400,180.06 |    0.5% |     13.77 | `single-thread / unbounded_channel`
|              190.46 |        5,250,545.05 |    2.8% |     13.75 | `multi-thread / bounded_channel`
|              177.13 |        5,645,667.10 |    1.2% |     13.52 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             923,345,814 | `single-thread / bounded_channel`
|             896,594,551 | `single-thread / unbounded_channel`
|              72,782,377 | `multi-thread / bounded_channel`
|              77,261,539 | `multi-thread / unbounded_channel`
