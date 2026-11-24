# limit-order-book

The goal of this project is to built a limit order book in C++.
I built an inital implimentation on the v1 branch, and I'm working to speed up and optimize the code in later versions. The improvements I made are detailed below.

## v1

Here are the benchmarks for the v1 implimentation:

```bash
Running ./build/release/order_book_bench
Run on (10 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 6144 KiB (x10)
Load Average: 1.37, 1.23, 1.28
------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------
BM_Add                               36969 ns        36932 ns        18983 items_per_second=27.0767M/s
BM_AddCancelSteady/1000               69.8 ns         69.7 ns     10034835 items_per_second=14.349M/s
BM_AddCancelSteady/10000              70.2 ns         70.1 ns      9962002 items_per_second=14.2715M/s
BM_AddCancelSteady/100000             70.1 ns         70.0 ns     10016312 items_per_second=14.2795M/s
BM_AddCancelSteady/1000000            71.9 ns         71.8 ns      9848890 items_per_second=13.9213M/s
BM_CancelFromQueueMiddle/10           45.3 ns         45.3 ns     15433290 items_per_second=22.0812M/s
BM_CancelFromQueueMiddle/100          39.3 ns         39.3 ns     17876753 items_per_second=25.439M/s
BM_CancelFromQueueMiddle/1000         38.7 ns         38.7 ns     18234343 items_per_second=25.8717M/s
BM_CancelFromQueueMiddle/10000        41.8 ns         41.7 ns     16982006 items_per_second=23.961M/s
BM_CancelFromQueueMiddle/100000       39.4 ns         39.4 ns     17740023 items_per_second=25.408M/s
BM_ReducePartial                      4.98 ns         4.98 ns    135522342 items_per_second=200.988M/s
BM_AddFullyFilling/0                  63.3 ns         63.2 ns     10922311 items_per_second=15.8176M/s no trades
BM_AddFullyFilling/1                  62.8 ns         62.7 ns     11098603 items_per_second=15.94M/s with trades
BM_SweepLevels/1                       418 ns          419 ns      1659554 items_per_second=2.38506M/s
BM_SweepLevels/10                      687 ns          688 ns      1007687 items_per_second=14.5289M/s
BM_SweepLevels/100                    3916 ns         3916 ns       194174 items_per_second=25.5366M/s
BM_SweepLevels/1000                  34691 ns        34623 ns        20510 items_per_second=28.8824M/s
BM_BestBid                           0.509 ns        0.508 ns   1344369971
BM_Front                             0.487 ns        0.486 ns   1286457280
BM_VolumeAt                           4.43 ns         4.42 ns    159459841
BM_Depth/1                            15.1 ns         15.0 ns     47019627
BM_Depth/10                           22.0 ns         21.9 ns     31824257
BM_Depth/100                           148 ns          148 ns      4426066
BM_MixedFlow                          6.38 ms         6.37 ms          108 items_per_second=15.6873M/s
```
