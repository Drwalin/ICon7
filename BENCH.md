
# Benchmark results

LTO: om

## Benchmark c186135 (Release)

### OpenSSL

```
drwalin@darch ~/C/g/T/I/release (master) [SIGINT]> time ./multi_test -t=1 -p=5 -con=1000 -ss=5 -s=1000 -serialize-load=1 -stats -payload=1200 -delay=47 -port=4567 -ssl
E 1970-01-01+06:00:12.963.80U+0 [86131] multi_testing.cpp:317 int main() : Iteration: 0 started
E 1970-01-01+06:00:12.966.49U+0 [86134] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Sender


E 1970-01-01+06:00:12.967.44U+0 [86133] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Receiver
E 1970-01-01+06:00:13.037.42U+0 [86131] multi_testing.cpp:445 int main() : Hosts pair 0 | failed 0 | connected peers: 1000 / 1000
E 1970-01-01+06:01:03.291.25U+0 [86131] multi_testing.cpp:520 int main() : Instantenous throughput: 119.320438 Kops (137.803126 MiBps)
E 1970-01-01+06:01:03.291.27U+0 [86131] multi_testing.cpp:529 int main() : Total throughput: 119.152154 Kops (137.608775 MiBps)
E 1970-01-01+06:01:03.350.15U+0 [86131] multi_testing.cpp:541 int main() : Latency [ms] | avg: 17.63  std: 63.08    p0: 2.74   p50: 11.65   p90: 13.41   p95: 15.12   p99: 170.98   p99.9: 1028.11   p100: 1487.49
E 1970-01-01+06:01:03.350.17U+0 [86131] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:01:03.350.18U+0 [86131] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 1970-01-01+06:01:03.350.18U+0 [86131] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 1970-01-01+06:01:03.402.35U+0 [86131] multi_testing.cpp:445 int main() : Hosts pair 1 | failed 0 | connected peers: 1000 / 1000
E 1970-01-01+06:01:53.641.81U+0 [86131] multi_testing.cpp:520 int main() : Instantenous throughput: 119.355296 Kops (137.843383 MiBps)
E 1970-01-01+06:01:53.641.84U+0 [86131] multi_testing.cpp:529 int main() : Total throughput: 119.158698 Kops (137.616332 MiBps)
E 1970-01-01+06:01:53.696.89U+0 [86131] multi_testing.cpp:541 int main() : Latency [ms] | avg: 17.12  std: 60.98    p0: 1.92   p50: 11.38   p90: 13.32   p95: 14.77   p99: 154.11   p99.9: 1003.75   p100: 1437.95
E 1970-01-01+06:01:53.696.91U+0 [86131] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:01:53.696.92U+0 [86131] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 1970-01-01+06:01:53.696.92U+0 [86131] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 1970-01-01+06:01:53.747.21U+0 [86131] multi_testing.cpp:445 int main() : Hosts pair 2 | failed 0 | connected peers: 1000 / 1000
E 1970-01-01+06:02:43.975.80U+0 [86131] multi_testing.cpp:520 int main() : Instantenous throughput: 119.381251 Kops (137.873358 MiBps)
E 1970-01-01+06:02:43.975.83U+0 [86131] multi_testing.cpp:529 int main() : Total throughput: 119.173778 Kops (137.633749 MiBps)
E 1970-01-01+06:02:44.029.52U+0 [86131] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.99  std: 61.01    p0: 2.11   p50: 11.17   p90: 13.27   p95: 15.35   p99: 156.05   p99.9: 1001.57   p100: 1429.05
E 1970-01-01+06:02:44.029.52U+0 [86131] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:02:44.029.53U+0 [86131] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 1970-01-01+06:02:44.029.53U+0 [86131] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 1970-01-01+06:02:44.079.93U+0 [86131] multi_testing.cpp:445 int main() : Hosts pair 3 | failed 0 | connected peers: 1000 / 1000
E 1970-01-01+06:03:34.339.37U+0 [86131] multi_testing.cpp:520 int main() : Instantenous throughput: 119.307569 Kops (137.788263 MiBps)
E 1970-01-01+06:03:34.339.39U+0 [86131] multi_testing.cpp:529 int main() : Total throughput: 119.163631 Kops (137.622030 MiBps)
E 1970-01-01+06:03:34.394.58U+0 [86131] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.40  std: 59.40    p0: 2.39   p50: 10.90   p90: 12.63   p95: 13.88   p99: 149.02   p99.9: 973.72   p100: 1410.38
E 1970-01-01+06:03:34.394.60U+0 [86131] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:03:34.394.60U+0 [86131] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 1970-01-01+06:03:34.394.61U+0 [86131] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 1970-01-01+06:03:34.444.34U+0 [86131] multi_testing.cpp:445 int main() : Hosts pair 4 | failed 0 | connected peers: 1000 / 1000
E 1970-01-01+06:04:24.656.08U+0 [86131] multi_testing.cpp:520 int main() : Instantenous throughput: 119.421469 Kops (137.919807 MiBps)
E 1970-01-01+06:04:24.656.10U+0 [86131] multi_testing.cpp:529 int main() : Total throughput: 119.179958 Kops (137.640886 MiBps)
E 1970-01-01+06:04:24.709.99U+0 [86131] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.24  std: 59.94    p0: 1.99   p50: 10.57   p90: 12.41   p95: 14.09   p99: 149.03   p99.9: 971.98   p100: 1464.98
E 1970-01-01+06:04:24.710.00U+0 [86131] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:04:24.710.01U+0 [86131] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 1970-01-01+06:04:24.710.01U+0 [86131] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 1970-01-01+06:04:24.739.88U+0 [86131] multi_testing.cpp:610 int main() : Iteration: 0 finished: received/sent = 25000000/25000000 ; returned/called = 5000000/5000000
E 1970-01-01+06:04:24.739.89U+0 [86131] multi_testing.cpp:628 int main() : Success

________________________________________________________
Executed in  251.78 secs    fish           external
   usr time  171.54 secs  191.00 micros  171.54 secs
   sys time  158.56 secs  997.00 micros  158.55 secs
```

## Benchmark  (Release)

### OpenSSL

```
drwalin@darch ~/C/g/T/I/release (master)> time ./multi_test -t=1 -p=5 -con=1000 -ss=5 -s=1000 -serialize-load=1 -stats -payload=1200 -delay=47 -port=4567 -ssl
E 2026-05-02+01:24:04.559.23U+0 [166294] multi_testing.cpp:317 int main() : Iteration: 0 started


E 2026-05-02+01:24:04.561.91U+0 [166297] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Sender
E 2026-05-02+01:24:04.562.85U+0 [166296] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Receiver
E 2026-05-02+01:24:04.645.08U+0 [166294] multi_testing.cpp:445 int main() : Hosts pair 0 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:24:54.670.73U+0 [166294] multi_testing.cpp:520 int main() : Instantenous throughput: 119.866107 Kops (138.433319 MiBps)
E 2026-05-02+01:24:54.670.77U+0 [166294] multi_testing.cpp:529 int main() : Total throughput: 119.667184 Kops (138.203582 MiBps)
E 2026-05-02+01:24:54.722.75U+0 [166294] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.76  std: 61.11    p0: 4.69   p50: 11.03   p90: 12.55   p95: 13.37   p99: 166.76   p99.9: 999.90   p100: 1445.86
E 2026-05-02+01:24:54.722.76U+0 [166294] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:24:54.722.76U+0 [166294] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:24:54.722.77U+0 [166294] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:24:54.772.85U+0 [166294] multi_testing.cpp:445 int main() : Hosts pair 1 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:25:44.783.60U+0 [166294] multi_testing.cpp:520 int main() : Instantenous throughput: 119.901366 Kops (138.474040 MiBps)
E 2026-05-02+01:25:44.783.61U+0 [166294] multi_testing.cpp:529 int main() : Total throughput: 119.698177 Kops (138.239377 MiBps)
E 2026-05-02+01:25:44.835.49U+0 [166294] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.68  std: 59.24    p0: 1.69   p50: 11.19   p90: 12.77   p95: 14.06   p99: 146.59   p99.9: 971.39   p100: 1415.80
E 2026-05-02+01:25:44.835.50U+0 [166294] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:25:44.835.50U+0 [166294] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:25:44.835.50U+0 [166294] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:25:44.885.07U+0 [166294] multi_testing.cpp:445 int main() : Hosts pair 2 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:26:34.883.28U+0 [166294] multi_testing.cpp:520 int main() : Instantenous throughput: 119.931568 Kops (138.508920 MiBps)
E 2026-05-02+01:26:34.883.30U+0 [166294] multi_testing.cpp:529 int main() : Total throughput: 119.719247 Kops (138.263710 MiBps)
E 2026-05-02+01:26:34.934.93U+0 [166294] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.58  std: 58.23    p0: 3.32   p50: 11.17   p90: 12.81   p95: 14.29   p99: 141.34   p99.9: 958.11   p100: 1443.46
E 2026-05-02+01:26:34.934.94U+0 [166294] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:26:34.934.94U+0 [166294] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:26:34.934.94U+0 [166294] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:26:34.984.18U+0 [166294] multi_testing.cpp:445 int main() : Hosts pair 3 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:27:24.985.18U+0 [166294] multi_testing.cpp:520 int main() : Instantenous throughput: 119.924036 Kops (138.500221 MiBps)
E 2026-05-02+01:27:24.985.20U+0 [166294] multi_testing.cpp:529 int main() : Total throughput: 119.728216 Kops (138.274068 MiBps)
E 2026-05-02+01:27:25.036.81U+0 [166294] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.82  std: 58.74    p0: 4.52   p50: 11.30   p90: 13.16   p95: 15.35   p99: 145.21   p99.9: 968.34   p100: 1411.73
E 2026-05-02+01:27:25.036.81U+0 [166294] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:27:25.036.82U+0 [166294] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:27:25.036.82U+0 [166294] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:27:25.085.87U+0 [166294] multi_testing.cpp:445 int main() : Hosts pair 4 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:28:15.069.45U+0 [166294] multi_testing.cpp:520 int main() : Instantenous throughput: 119.966082 Kops (138.548780 MiBps)
E 2026-05-02+01:28:15.069.48U+0 [166294] multi_testing.cpp:529 int main() : Total throughput: 119.742236 Kops (138.290260 MiBps)
E 2026-05-02+01:28:15.121.24U+0 [166294] multi_testing.cpp:541 int main() : Latency [ms] | avg: 16.45  std: 57.32    p0: 3.22   p50: 11.16   p90: 12.72   p95: 13.92   p99: 136.04   p99.9: 944.99   p100: 1369.10
E 2026-05-02+01:28:15.121.25U+0 [166294] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:28:15.121.25U+0 [166294] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:28:15.121.26U+0 [166294] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:28:15.151.67U+0 [166294] multi_testing.cpp:610 int main() : Iteration: 0 finished: received/sent = 25000000/25000000 ; returned/called = 5000000/5000000
E 2026-05-02+01:28:15.151.69U+0 [166294] multi_testing.cpp:628 int main() : Success

________________________________________________________
Executed in  250.61 secs    fish           external
   usr time  136.76 secs    0.00 millis  136.76 secs
   sys time  163.71 secs    1.05 millis  163.71 secs
```

### BoringSSL

```
drwalin@darch ~/C/g/T/I/release (master)> time ./multi_test -t=1 -p=5 -con=1000 -ss=5 -s=1000 -serialize-load=1 -stats -payload=1200 -delay=47 -port=4567 -ssl
E 2026-05-02+01:18:32.333.12U+0 [165200] multi_testing.cpp:317 int main() : Iteration: 0 started


E 2026-05-02+01:18:32.334.63U+0 [165203] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Sender
E 2026-05-02+01:18:32.334.71U+0 [165202] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Receiver
E 2026-05-02+01:18:32.394.63U+0 [165200] multi_testing.cpp:445 int main() : Hosts pair 0 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:19:22.361.92U+0 [165200] multi_testing.cpp:520 int main() : Instantenous throughput: 120.006202 Kops (138.595115 MiBps)
E 2026-05-02+01:19:22.361.94U+0 [165200] multi_testing.cpp:529 int main() : Total throughput: 119.860000 Kops (138.426266 MiBps)
E 2026-05-02+01:19:22.413.44U+0 [165200] multi_testing.cpp:541 int main() : Latency [ms] | avg: 9.69  std: 33.13    p0: 0.98   p50: 7.18   p90: 8.19   p95: 8.43   p99: 12.62   p99.9: 596.77   p100: 912.17
E 2026-05-02+01:19:22.413.45U+0 [165200] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:19:22.413.45U+0 [165200] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:19:22.413.45U+0 [165200] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:19:22.447.22U+0 [165200] multi_testing.cpp:445 int main() : Hosts pair 1 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:20:12.391.94U+0 [165200] multi_testing.cpp:520 int main() : Instantenous throughput: 120.061345 Kops (138.658799 MiBps)
E 2026-05-02+01:20:12.391.96U+0 [165200] multi_testing.cpp:529 int main() : Total throughput: 119.894445 Kops (138.466046 MiBps)
E 2026-05-02+01:20:12.443.72U+0 [165200] multi_testing.cpp:541 int main() : Latency [ms] | avg: 9.66  std: 31.42    p0: 0.92   p50: 7.30   p90: 8.40   p95: 8.71   p99: 13.16   p99.9: 563.33   p100: 871.46
E 2026-05-02+01:20:12.443.73U+0 [165200] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:20:12.443.73U+0 [165200] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:20:12.443.73U+0 [165200] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:20:12.477.88U+0 [165200] multi_testing.cpp:445 int main() : Hosts pair 2 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:21:02.450.35U+0 [165200] multi_testing.cpp:520 int main() : Instantenous throughput: 119.994934 Kops (138.582101 MiBps)
E 2026-05-02+01:21:02.450.37U+0 [165200] multi_testing.cpp:529 int main() : Total throughput: 119.883036 Kops (138.452870 MiBps)
E 2026-05-02+01:21:02.502.83U+0 [165200] multi_testing.cpp:541 int main() : Latency [ms] | avg: 9.71  std: 31.51    p0: 0.95   p50: 7.34   p90: 8.41   p95: 8.68   p99: 12.47   p99.9: 562.83   p100: 869.82
E 2026-05-02+01:21:02.502.84U+0 [165200] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:21:02.502.84U+0 [165200] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:21:02.502.85U+0 [165200] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:21:02.538.83U+0 [165200] multi_testing.cpp:445 int main() : Hosts pair 3 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:21:52.526.59U+0 [165200] multi_testing.cpp:520 int main() : Instantenous throughput: 119.958052 Kops (138.539506 MiBps)
E 2026-05-02+01:21:52.526.61U+0 [165200] multi_testing.cpp:529 int main() : Total throughput: 119.866554 Kops (138.433834 MiBps)
E 2026-05-02+01:21:52.578.28U+0 [165200] multi_testing.cpp:541 int main() : Latency [ms] | avg: 9.71  std: 32.27    p0: 2.23   p50: 7.27   p90: 8.30   p95: 8.56   p99: 12.21   p99.9: 570.75   p100: 907.95
E 2026-05-02+01:21:52.578.29U+0 [165200] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:21:52.578.29U+0 [165200] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:21:52.578.30U+0 [165200] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:21:52.612.13U+0 [165200] multi_testing.cpp:445 int main() : Hosts pair 4 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:22:42.591.67U+0 [165200] multi_testing.cpp:520 int main() : Instantenous throughput: 119.977715 Kops (138.562215 MiBps)
E 2026-05-02+01:22:42.591.69U+0 [165200] multi_testing.cpp:529 int main() : Total throughput: 119.862029 Kops (138.428609 MiBps)
E 2026-05-02+01:22:42.643.41U+0 [165200] multi_testing.cpp:541 int main() : Latency [ms] | avg: 9.56  std: 31.61    p0: 0.92   p50: 7.21   p90: 8.22   p95: 8.45   p99: 11.53   p99.9: 565.38   p100: 872.67
E 2026-05-02+01:22:42.643.42U+0 [165200] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:22:42.643.42U+0 [165200] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:22:42.643.43U+0 [165200] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:22:42.658.90U+0 [165200] multi_testing.cpp:610 int main() : Iteration: 0 finished: received/sent = 25000000/25000000 ; returned/called = 5000000/5000000
E 2026-05-02+01:22:42.658.91U+0 [165200] multi_testing.cpp:628 int main() : Success

________________________________________________________
Executed in  250.33 secs    fish           external
   usr time   84.02 secs    0.00 millis   84.02 secs
   sys time  147.04 secs    1.01 millis  147.04 secs
```

### No SSL

```
drwalin@darch ~/C/g/T/I/release (master)> time ./multi_test -t=1 -p=5 -con=1000 -ss=5 -s=1000 -serialize-load=1 -stats -payload=1200 -delay=47 -port=4567 
E 2026-05-02+01:29:15.884.97U+0 [166987] multi_testing.cpp:317 int main() : Iteration: 0 started


E 2026-05-02+01:29:15.886.19U+0 [166990] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Sender
E 2026-05-02+01:29:15.886.26U+0 [166989] multi_testing.cpp:369 void main()::CommandPrintThreadId::Execute() : Thread Receiver
E 2026-05-02+01:29:15.944.23U+0 [166987] multi_testing.cpp:445 int main() : Hosts pair 0 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:30:05.793.14U+0 [166987] multi_testing.cpp:520 int main() : Instantenous throughput: 120.292245 Kops (138.925465 MiBps)
E 2026-05-02+01:30:05.793.16U+0 [166987] multi_testing.cpp:529 int main() : Total throughput: 120.150030 Kops (138.761222 MiBps)
E 2026-05-02+01:30:05.842.41U+0 [166987] multi_testing.cpp:541 int main() : Latency [ms] | avg: 5.32  std: 0.53    p0: 4.12   p50: 5.24   p90: 6.02   p95: 6.24   p99: 6.90   p99.9: 8.35   p100: 9.59
E 2026-05-02+01:30:05.842.43U+0 [166987] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:30:05.842.43U+0 [166987] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:30:05.842.44U+0 [166987] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:30:05.875.40U+0 [166987] multi_testing.cpp:445 int main() : Hosts pair 1 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:30:55.793.92U+0 [166987] multi_testing.cpp:520 int main() : Instantenous throughput: 120.126146 Kops (138.733637 MiBps)
E 2026-05-02+01:30:55.793.93U+0 [166987] multi_testing.cpp:529 int main() : Total throughput: 120.074835 Kops (138.674378 MiBps)
E 2026-05-02+01:30:55.843.54U+0 [166987] multi_testing.cpp:541 int main() : Latency [ms] | avg: 5.29  std: 0.52    p0: 0.64   p50: 5.22   p90: 5.99   p95: 6.19   p99: 6.70   p99.9: 7.50   p100: 8.29
E 2026-05-02+01:30:55.843.55U+0 [166987] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:30:55.843.55U+0 [166987] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:30:55.843.56U+0 [166987] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:30:55.871.78U+0 [166987] multi_testing.cpp:445 int main() : Hosts pair 2 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:31:45.771.24U+0 [166987] multi_testing.cpp:520 int main() : Instantenous throughput: 120.170993 Kops (138.785431 MiBps)
E 2026-05-02+01:31:45.771.26U+0 [166987] multi_testing.cpp:529 int main() : Total throughput: 120.067695 Kops (138.666132 MiBps)
E 2026-05-02+01:31:45.820.53U+0 [166987] multi_testing.cpp:541 int main() : Latency [ms] | avg: 5.30  std: 0.51    p0: 4.12   p50: 5.24   p90: 6.00   p95: 6.20   p99: 6.70   p99.9: 7.45   p100: 8.21
E 2026-05-02+01:31:45.820.55U+0 [166987] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:31:45.820.55U+0 [166987] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:31:45.820.55U+0 [166987] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:31:45.848.77U+0 [166987] multi_testing.cpp:445 int main() : Hosts pair 3 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:32:35.803.68U+0 [166987] multi_testing.cpp:520 int main() : Instantenous throughput: 120.038687 Kops (138.632631 MiBps)
E 2026-05-02+01:32:35.803.69U+0 [166987] multi_testing.cpp:529 int main() : Total throughput: 120.031552 Kops (138.624392 MiBps)
E 2026-05-02+01:32:35.852.79U+0 [166987] multi_testing.cpp:541 int main() : Latency [ms] | avg: 5.31  std: 0.53    p0: 0.71   p50: 5.24   p90: 6.02   p95: 6.24   p99: 6.92   p99.9: 7.54   p100: 8.17
E 2026-05-02+01:32:35.852.80U+0 [166987] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:32:35.852.80U+0 [166987] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:32:35.852.80U+0 [166987] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000


E 2026-05-02+01:32:35.880.67U+0 [166987] multi_testing.cpp:445 int main() : Hosts pair 4 | failed 0 | connected peers: 1000 / 1000
E 2026-05-02+01:33:25.762.64U+0 [166987] multi_testing.cpp:520 int main() : Instantenous throughput: 120.213239 Kops (138.834221 MiBps)
E 2026-05-02+01:33:25.762.66U+0 [166987] multi_testing.cpp:529 int main() : Total throughput: 120.044755 Kops (138.639640 MiBps)
E 2026-05-02+01:33:25.814.34U+0 [166987] multi_testing.cpp:541 int main() : Latency [ms] | avg: 5.30  std: 0.50    p0: 4.09   p50: 5.24   p90: 5.98   p95: 6.17   p99: 6.61   p99.9: 7.45   p100: 8.03
E 2026-05-02+01:33:25.814.36U+0 [166987] multi_testing.cpp:547 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:33:25.814.36U+0 [166987] multi_testing.cpp:551 int main() : Waiting to finish message transmission
E 2026-05-02+01:33:25.814.37U+0 [166987] multi_testing.cpp:577 int main() : received/sent = 5000000/5000000 ; returned/called = 1000000/1000000
E 2026-05-02+01:33:25.830.38U+0 [166987] multi_testing.cpp:610 int main() : Iteration: 0 finished: received/sent = 25000000/25000000 ; returned/called = 5000000/5000000
E 2026-05-02+01:33:25.830.40U+0 [166987] multi_testing.cpp:628 int main() : Success

________________________________________________________
Executed in  249.95 secs    fish           external
   usr time   37.50 secs    0.00 millis   37.50 secs
   sys time  132.89 secs    1.02 millis  132.89 secs
```

