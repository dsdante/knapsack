open System
open System.Diagnostics
open System.IO
open Microsoft.FSharp.Collections

let inline (|?) (a: 'a option) b = if a.IsSome then a.Value else b

let filename =
    Environment.GetCommandLineArgs()
    |> Seq.skip(1)
    |> Seq.tryHead
    |? "input"

let values =
    File.ReadLines(filename)
    |> Seq.map Double.Parse
    |> Seq.toArray

let limit = values.[0]

let items = values.[1..]

let threadDepth = Math.Log(float Environment.ProcessorCount, 2.) |> ceil

let rec knapsack (depth: int, sum: float, mask: byref<Int64>) =
    if depth >= items.Length then
        sum
    else
        mask <- mask <<< 1
        let mutable maskB = mask ||| 0x1L
        let a = knapsack(depth+1, sum, &mask)
        let sumB = sum + items.[depth]
        if sumB > limit then
            a
        else
            let b = knapsack(depth+1, sumB, &maskB)
            if a > b then
                a
            else
                mask <- maskB
                b

[<EntryPoint>]
let main _ =
    let watch = Stopwatch.StartNew()
    let mutable mask = 0x0L
    let result = knapsack(0, 0., &mask)
    printfn "%A" result
    watch.Stop()
    printfn "%A" watch.Elapsed.TotalSeconds
    0 
