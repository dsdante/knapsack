using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

class Program
{
    static double limit;  // knapsack size
    static double[] items;
    static int threadDepth = 0;  // num of threads = 2^branch_depth

    // Synchronous recursive routine
    static double Knapsack(int depth, double sum, ref Int64 mask)
    {
        if (depth >= items.Length)
            return sum;

        mask <<= 1;
        Int64 maskB = mask | 0x1;
        double a = Knapsack(depth+1, sum, ref mask);  // try without the element #depth

        sum += items[depth];
        if (sum > limit)  // pruning
            return a;
        double b = Knapsack(depth+1, sum, ref maskB);  // try with the element #depth

        if (a > b)
            return a;

        mask = maskB;
        return b;
    }

    // Asynchronous recursive routine
    static (double sum, Int64 mask) KnapsackAsync(int depth, double sum, Int64 mask)
    {
        if (depth >= items.Length)
            return (sum, mask);

        if (depth == threadDepth)  // no more branching
            return (Knapsack(depth, sum, ref mask), mask);
        
        mask <<= 1;
        var taskA = Task.Run(() => KnapsackAsync(depth+1, sum, mask));  // try without the element #depth

        sum += items[depth];
        if (sum > limit)
            return taskA.Result;
        var b = KnapsackAsync(depth+1, sum, mask | 0x1);  // try with the element #depth
        var a = taskA.Result;

        if (a.sum > b.sum)
            return a;

        return b;
    }

    static void Main(string[] args)
    {
        string filename = args.FirstOrDefault() ?? "input";
        double[] values = File.ReadLines(filename).Select(double.Parse).ToArray();
        limit = values[0];
        items = values.Skip(1).ToArray();

        Stopwatch watch = Stopwatch.StartNew();
        // Keeping the bigger items at the end allows pruning
        // at the later stages, thus balancing the tree traversal
        Array.Sort(items);
        int cores = Environment.ProcessorCount;
        if (cores > 1) // the neares upper power of 2 allows to utilize all cores
            threadDepth = (int)Math.Ceiling(Math.Log(cores, 2));
        (double sum, Int64 mask) = KnapsackAsync(0, 0, 0x0);
        watch.Stop();

        Console.WriteLine("time: {0:0.00}", watch.Elapsed.TotalSeconds);
        Console.WriteLine("elements: {0}", Convert.ToString(mask, 2).Count(c => c == '1'));
        Console.WriteLine("sum: {0:0.000000000}", sum);
        for (int i = items.Length-1; i >= 0; i--)
        {
            if ((mask & 0x1) != 0)
                Console.Write(items[i] + " ");
            mask >>= 1;
        }
        Console.WriteLine();
    }
}
