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
|               15.77 |       63,410,053.27 |    0.1% |     13.76 | `single-thread / bounded_channel`
|               14.94 |       66,934,313.69 |    0.3% |     13.78 | `single-thread / unbounded_channel`
|              197.37 |        5,066,566.82 |    1.4% |     13.72 | `multi-thread / bounded_channel`
|              176.44 |        5,667,726.19 |    0.7% |     13.66 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             883,620,864 | `single-thread / bounded_channel`
|             930,573,110 | `single-thread / unbounded_channel`
|              70,832,579 | `multi-thread / bounded_channel`
|              78,483,162 | `multi-thread / unbounded_channel`
