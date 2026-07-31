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
|               15.41 |       64,891,208.38 |    0.3% |     13.79 | `single-thread / bounded_channel`
|               15.46 |       64,666,566.43 |    0.1% |     13.75 | `single-thread / unbounded_channel`
|              181.24 |        5,517,658.95 |    2.2% |     13.68 | `multi-thread / bounded_channel`
|              190.88 |        5,238,782.32 |    1.3% |     13.38 | `multi-thread / unbounded_channel`

| Total Messages Enqueued | xtd::channel throughput 
|------------------------:|:-------------------------
|             905,988,353 | `single-thread / bounded_channel`
|             899,504,568 | `single-thread / unbounded_channel`
|              76,265,556 | `multi-thread / bounded_channel`
|              71,661,926 | `multi-thread / unbounded_channel`
