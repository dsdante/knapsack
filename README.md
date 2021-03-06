## Knapsack problem

The [knapsack problem](https://en.wikipedia.org/wiki/Knapsack_problem) is to find the set of non-negative `items` with the maximum sum below `limit`.

The goal is to modify the naive solution to make it solve the problem with N=30 in a minimum time.

Input file format:  
`double limit`  
`double item[0]`  
`double item[1]`  
`...`  
`double item[N-1]`

knapsack-gen.c: input data generator  
knapsack-naive.c: a naive implementation  
run-all.sh: run all versions with performance measurement

Requirements: CMake, GCC, Rust, Mono

A typical ratio of running time:  
assembly = 1  
C = 2.0  
Rust = 2.1  
C# = 3.5
