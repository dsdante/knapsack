open System
open System.IO
open System.Numerics;
open System.Threading.Tasks;

let filename = Environment.GetCommandLineArgs() |> Array.tryItem 1 |> Option.defaultValue "input"
let lines = File.ReadLines(filename) |> Seq.map double |> Seq.toList
let limit = lines |> Seq.head  // Knapsack size
let items = lines |> Seq.skip 1 |> Seq.sort |> Seq.toArray  // Sorting balances the recursion tree.
let threadDepth = Environment.ProcessorCount |> Math.Log2 |> Math.Ceiling |> int

// Parallel recursive knapsack calculation
//   depth: current recursion depth, i.e. the number of the considered items
//   sum: the sum of the taken items among the considered ones
//   mask: the bit mask of the taken items
//   Returns the best sum for this branch of recursion and its bit mask.
let rec knapsack depth sum mask =
    let sumB = sum + items.[depth]
    if sumB > limit then  // Recursion pruning
        sum, mask <<< items.Length - depth
    elif depth = items.Length - 1 then
        sumB, mask <<< 1 ||| 1UL
    elif depth < threadDepth then
        // Spawn a thread.
        let a = Task.Run(fun () -> knapsack (depth + 1) sum (mask <<< 1))
        max (knapsack (depth + 1) sumB (mask <<< 1 ||| 1UL))
            a.Result
    else
        max (knapsack (depth + 1) sum (mask <<< 1))
            (knapsack (depth + 1) sumB (mask <<< 1 ||| 1UL))

printfn $"Running '{filename}'..."
let sum, mask = knapsack 0 0 0UL

printfn $"Sum: {sum:F9} / {limit}";
printfn $"Used items: {BitOperations.PopCount(mask)} / {items.Length}";
items |> Array.rev |> Array.iteri(fun i item ->
    if mask >>> i &&& 1UL = 1UL then
        printfn $"{item:F9}")
