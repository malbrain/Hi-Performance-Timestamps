# Hi-Performance-Timestamps
Transaction Commitment Timestamp generator for a thousand cores

The basic idea is to create and deliver timestamps for client transactions on the order of a billion timestamp requests per second.  The generator must have an extremely short execution path that doesn't require atomic operations for every request.

#ifdef SCAN
    printf("Table Scan\n");
#endif
#ifdef QUEUE
    printf("FIFO Queue\n");
#endif
#ifdef ATOMIC
    printf("Atomic Incr\n");
#endif
#ifdef ALIGN
    printf("Atomic Aligned 64\n");
#endif
#ifdef RDTSC
    printf("TSC COUNT: New  Epochs = %" PRIu64 "\n", rdtscEpochs);
#endif
#ifdef CLOCK
    printf("Hi Res Timer\n");
#endif

As a baseline here are the timing results for one billion requests for a new timestamp using an atomic increment of a single common timestamp by threads 1 thru  7 concurrently.

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

5 threads:
------------------------

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

6 threads:
------------------------

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

7 threads:
------------------------

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

One answer to this deteriorating situation is to derive each timestamp value from the RDTSC hardware counter, and an atomic ops each second. 

1 thread:
------------------------

$ ./a.out 1 1000000000
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 1000000000 timestamps
Begin client 1
client 1 count = 1000000000 Out of Order = 0
TSC COUNT: New  Epochs = 7
 real 0m7.253s
 user 0m7.266s
 sys  0m0.000s

2 threads:
------------------------

$ ./a.out 2 500000000
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 500000000 timestamps
Begin client 1
thread 2 launched for 500000000 timestamps
Begin client 2
client 1 count = 500000000 Out of Order = 0
client 2 count = 500000000 Out of Order = 0
TSC COUNT: New  Epochs = 18
 real 0m11.097s
 user 0m20.391s
 sys  0m0.000s

3 threads:
------------------------

$ ./a.out 3 333333333
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 333333333 timestamps
Begin client 1
thread 2 launched for 333333333 timestamps
Begin client 2
thread 3 launched for 333333333 timestamps
Begin client 3
client 3 count = 333333333 Out of Order = 0
client 2 count = 333333333 Out of Order = 0
client 1 count = 333333333 Out of Order = 0
TSC COUNT: New  Epochs = 39
 real 0m15.102s
 user 0m43.969s
 sys  0m0.000s

4 threads:
------------------------

$ ./a.out 4 250000000
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 250000000 timestamps
Begin client 1
thread 2 launched for 250000000 timestamps
Begin client 2
thread 3 launched for 250000000 timestamps
Begin client 3
thread 4 launched for 250000000 timestamps
Begin client 4
client 3 count = 250000000 Out of Order = 0
client 4 count = 250000000 Out of Order = 0
client 1 count = 250000000 Out of Order = 0
client 2 count = 250000000 Out of Order = 0
TSC COUNT: New  Epochs = 49
 real 0m14.104s
 user 0m55.844s
 sys  0m0.000s

5 threads:
------------------------

$ ./a.out 5 200000000
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 200000000 timestamps
Begin client 1
thread 2 launched for 200000000 timestamps
Begin client 2
thread 3 launched for 200000000 timestamps
Begin client 3
thread 4 launched for 200000000 timestamps
Begin client 4
thread 5 launched for 200000000 timestamps
Begin client 5
client 3 count = 200000000 Out of Order = 0
client 2 count = 200000000 Out of Order = 0
client 1 count = 200000000 Out of Order = 0
client 4 count = 200000000 Out of Order = 0
client 5 count = 200000000 Out of Order = 0
TSC COUNT: New  Epochs = 58
 real 0m13.435s
 user 1m6.109s
 sys  0m0.000s

6 threads:
------------------------

$ ./a.out 6 166666666
size of Timestamp = 8, TsEpoch = 16
thread 1 launched for 166666666 timestamps
Begin client 1
thread 2 launched for 166666666 timestamps
Begin client 2
thread 3 launched for 166666666 timestamps
Begin client 3
thread 4 launched for 166666666 timestamps
Begin client 4
thread 5 launched for 166666666 timestamps
Begin client 5
thread 6 launched for 166666666 timestamps
Begin client 6
client 6 count = 166666666 Out of Order = 0
client 1 count = 166666666 Out of Order = 0
client 2 count = 166666666 Out of Order = 0
client 5 count = 166666666 Out of Order = 0
client 4 count = 166666666 Out of Order = 0
client 3 count = 166666666 Out of Order = 0
TSC COUNT: New  Epochs = 58
 real 0m12.039s
 user 1m8.016s
 sys  0m0.031s

------------------------

