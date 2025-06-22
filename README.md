# Fibonacci Hashing

This repository contains a small benchmark for analysing **Fibonacci hashing** versus the classic modulo-based hash. The experiment measures

- insertion, retrieval and deletion times
- collision behaviour (average and maximum cluster length)
- memory usage
- sensitivity to different key patterns (random, sequential and clustered)

## Building

Compile `main.cpp` with a C++17 compiler:

```bash
g++ -std=c++17 -O2 main.cpp -o main
```

## Running

Execute the generated binary and enter one or more key counts when prompted.
Separate multiple values with spaces. The program will show metrics for each
size across random, sequential and clustered datasets:

```bash
./main
```

The output reports the load factor, average chain length, maximum chain length
and execution times (in microseconds) for both hashing strategies. Execution
times are averaged over three runs to reduce variance.

Additionally, the program writes these metrics to `results.csv` for each input
size so they can be opened in spreadsheet software like Excel for further
analysis.
