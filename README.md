# Hi-Performance-Timestamps
Transaction Commitment Timestamp generator for a thousand cores

The basic idea is to create and deliver timestamps for client transactions on the order of a billion timestamp requests per second.  The generator must have an extremely short execution path that doesn't require atomic operations.

As a baseline here are the timing results for one billion requests for a new timestamp using an atomic increment of a common timestamp.  The data illustrates the steady deteriorating of throughput caused by the serialized hardware implementation of atomics.

No contention, 1 thread:
------------------------

Hi-Performance-Timestamps\timestamps>standalone.exe 1 1000000000
thread 1 launched for 1000000000 timestamps
Begin client 1
Atomic Incr
 real 0m6.260s -- 6ns per timestamp
 user 0m6.250s
 sys  0m0.000s

2 threads:
------------------------

Hi-Performance-Timestamps\timestamps>standalone.exe 2 500000000
thread 1 launched for 500000000 timestamps
thread 2 launched for 500000000 timestamps
Begin client 1
Begin client 2
Atomic Incr
 real 0m20.167s -- 20ns
 user 0m40.250s
 sys  0m0.000s

3 threads:
------------------------

Hi-Performance-Timestamps\timestamps>standalone.exe 3 333333333
thread 1 launched for 333333333 timestamps
thread 2 launched for 333333333 timestamps
Begin client 2
Begin client 3
thread 3 launched for 333333333 timestamps
Begin client 1
Atomic Incr
 real 0m25.531s
 user 1m16.437s
 sys  0m0.000s

4 threads:
------------------------

Hi-Performance-Timestamps\timestamps>standalone.exe 4 250000000
thread 1 launched for 250000000 timestamps
thread 2 launched for 250000000 timestamps
thread 3 launched for 250000000 timestamps
Begin client 2
Begin client 3
thread 4 launched for 250000000 timestamps
Begin client 1
Begin client 4
Atomic Incr
 real 0m18.416s
 user 1m12.437s
 sys  0m0.000s

Hi-Performance-Timestamps\timestamps>standalone.exe 5 200000000
thread 1 launched for 200000000 timestamps
thread 2 launched for 200000000 timestamps
Begin client 1
Begin client 2
thread 3 launched for 200000000 timestamps
Begin client 3
thread 4 launched for 200000000 timestamps
Begin client 4
thread 5 launched for 200000000 timestamps
Begin client 5
Atomic Incr
 real 0m24.651s
 user 2m2.343s
 sys  0m0.000s

Hi-Performance-Timestamps\timestamps>standalone.exe 6 166666666
thread 1 launched for 166666666 timestamps
thread 2 launched for 166666666 timestamps
Begin client 1
Begin client 2
thread 3 launched for 166666666 timestamps
Begin client 3
thread 4 launched for 166666666 timestamps
Begin client 4
Begin client 5
thread 5 launched for 166666666 timestamps
thread 6 launched for 166666666 timestamps
Begin client 6
Atomic Incr
 real 0m24.008s
 user 2m22.546s
 sys  0m0.000s

Hi-Performance-Timestamps\timestamps>standalone.exe 7 142857145
thread 1 launched for 142857145 timestamps
thread 2 launched for 142857145 timestamps
Begin client 2
Begin client 1
thread 3 launched for 142857145 timestamps
Begin client 3
thread 4 launched for 142857145 timestamps
Begin client 4
thread 5 launched for 142857145 timestamps
Begin client 5
thread 6 launched for 142857145 timestamps
Begin client 6
thread 7 launched for 142857145 timestamps
Begin client 7
Atomic Incr
 real 0m25.329s
 user 2m53.109s
 sys  0m0.047s

