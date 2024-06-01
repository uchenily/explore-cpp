# Benchmark

## environment

- cpu: Intel(R) Xeon(R) CPU E5-2660 v3 @ 2.60GHz
- kernel: 6.8.8-arch1-1
- g++: (GCC) 14.1.1 20240507
- wrk: 4.2.0 [epoll] Copyright (C) 2012 Will Glozer

## benchmark_st.cpp

```bash
$ date
Sun Jun  2 12:16:52 AM CST 2024
$ wrk -t4 -c100 -d30s --latency http://127.0.0.1:8000
Running 30s test @ http://127.0.0.1:8000
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   565.19us   92.77us   9.11ms   90.81%
    Req/Sec    43.28k     2.50k   48.93k    69.58%
  Latency Distribution
     50%  549.00us
     75%  573.00us
     90%  618.00us
     99%    1.04ms
  5167791 requests in 30.00s, 438.63MB read
Requests/sec: 172257.14
Transfer/sec:     14.62MB
```
