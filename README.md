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

A typical ratio of running time is:  
assembly = 1  
C = 1.7  
Rust = 1.8  
C# = 3
