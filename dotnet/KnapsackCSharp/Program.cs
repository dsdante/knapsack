string filename = args.FirstOrDefault() ?? "input";

var values = File.ReadLines(filename).Select(double.Parse).ToArray();
var limit = values[0];  // knapsack size
var items = values[1..];
Array.Sort(items);  // Allows pruning at the later stages, thus balancing the recursion tree.
int threadDepth = (int)Math.Ceiling(Math.Log2(Environment.ProcessorCount));  // number of threads = 2 ^ branch_depth

Console.WriteLine($"Running '{filename}'...");
(double sum, ulong mask) = KnapsackParallel(0, 0, 0x0);

Console.WriteLine($"Sum: {sum:F9} / {limit}");
Console.WriteLine($"Used items: {System.Numerics.BitOperations.PopCount(mask)} / {items.Length}");
for (int i = 0; i < items.Length; i++)
    if ((mask >> i & 0x01) != 0)
        Console.WriteLine($"{items[^(i+1)]:F9}");

// Recursive knapsack calculation
//   depth: current recursion depth, i.e. the number of the considered items
//   sum: the sum of the taken items among the considered ones
//   mask: the bit mask of the taken items
//   returns the best sum for this branch of recursion and its bit mask
(double sum, ulong mask) Knapsack(int depth, double sum, ulong mask)
{
    // Recursion pruning
    var sumB = sum + items[depth];
    if (sumB > limit)
        return (sum, mask << items.Length - depth);

    mask <<= 1;
    var maskB = mask | 0x01;
    depth++;

    if (depth == items.Length)
        return (sumB, maskB);

    (sum, mask) = Knapsack(depth, sum, mask);  // try without the element [depth]
    (sumB, maskB) = Knapsack(depth, sumB, maskB);  // try with the element [depth]

    if (sum > sumB)
        return (sum, mask);
    return (sumB, maskB);
}

// Parallel recursive knapsack calculation
(double sum, ulong mask) KnapsackParallel(int depth, double sum, ulong mask)
{
    if (depth == threadDepth)
        return Knapsack(depth, sum, mask);

    // Recursion pruning
    var sumB = sum + items[depth];
    if (sumB > limit)
        return (sum, mask << items.Length - depth);

    mask <<= 1;
    var maskB = mask | 0x01;
    depth++;

    if (depth == items.Length)
        return (sumB, maskB);

    var taskA = Task.Run(() => KnapsackParallel(depth, sum, mask));  // try without the element [depth]
    (sumB, maskB) = KnapsackParallel(depth, sumB, maskB);  // try with the element [depth]
    (sum, mask) = taskA.Result;

    if (sum > sumB)
        return (sum, mask);
    return (sumB, maskB);
}
