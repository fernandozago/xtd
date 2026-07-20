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
|               17.09 |       58,519,601.40 |    0.1% |     13.73 | `single-thread / bounded_channel`
|               13.53 |       73,883,719.23 |    0.8% |     13.79 | `single-thread / unbounded_channel`
|              209.70 |        4,768,665.52 |    1.9% |     13.79 | `multi-thread / bounded_channel`
|              208.50 |        4,796,091.47 |    2.6% |     13.88 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             813,640,041 | `single-thread / bounded_channel`
|           1,027,446,397 | `single-thread / unbounded_channel`
|              66,429,434 | `multi-thread / bounded_channel`
|              67,556,721 | `multi-thread / unbounded_channel`
