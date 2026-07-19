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
|               15.08 |       66,305,046.18 |    0.8% |     13.66 | `single-thread / bounded_channel`
|               14.31 |       69,864,762.75 |    0.6% |     13.78 | `single-thread / unbounded_channel`
|              149.89 |        6,671,765.39 |    0.8% |     13.82 | `multi-thread / bounded_channel`
|              150.82 |        6,630,342.55 |    1.5% |     13.66 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             914,853,785 | `single-thread / bounded_channel`
|             974,762,434 | `single-thread / unbounded_channel`
|              93,274,786 | `multi-thread / bounded_channel`
|              91,760,046 | `multi-thread / unbounded_channel`
