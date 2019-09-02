using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

class Program
{
    static string filename = "input";  // May be overridden with an argument.
    static double[] items;
    static double limit;  // Knapsack size
    static int threadDepth = 0;  // Number of threads = 2^branch_depth

    // Recursive knapsack calculation
    //   depth: current recursion depth, i.e. the number of the considered items
    //   sum: the sum of the taken items among considered
    //   mask: the bit mask of the taken items
    //   returns the best sum for this branch of recursion
    static double Knapsack(int depth, double sum, ref Int64 mask)
    {
        if (depth == items.Length)
            return sum;

        double sumB = sum + items[depth];
        if (sumB > limit)
        {  // Recursion pruning
            mask <<= items.Length - depth;
            return sum;
        }

        mask <<= 1;
        Int64 maskB = mask | 0x1U;

        sum  = Knapsack(depth+1, sum, ref mask);
        sumB = Knapsack(depth+1, sumB, ref maskB);

        if (sumB > sum) {
            sum = sumB;
            mask = maskB;
        }

        return sum;
    }

    // Asynchronous recursive knapsack calculation
    static (double sum, Int64 mask) KnapsackAsync(int depth, double sum, Int64 mask)
    {
        if (depth == items.Length)
            return (sum, mask);

        if (depth == threadDepth)
        {
            sum = Knapsack(depth, sum, ref mask);
            return (sum, mask);
        }

        if (sum + items[depth] > limit)
        {  // Recursion pruning
            mask <<= items.Length - depth;
            return (Knapsack(depth, sum, ref mask), mask);
        }

        mask <<= 1;
        var taskA = Task.Run(() => KnapsackAsync(depth+1, sum, mask));  // Try without the element #depth
        var b = KnapsackAsync(depth+1, sum + items[depth], mask | 0x1);  // Try with the element #depth
        var a = taskA.Result;

        if (b.sum > a.sum)
            return b;
        return a;
    }

    static void Main(string[] args)
    {
        filename = args.FirstOrDefault() ?? filename;
        Console.WriteLine($"Running '{filename}'...");
        double[] values = File.ReadLines(filename).Select(double.Parse).ToArray();
        limit = values[0];
        items = values.Skip(1).ToArray();

        // Keeping the bigger items at the end allows pruning at the later stages,
        // thus balancing the recursion tree
        Array.Sort(items);
        int cores = Environment.ProcessorCount;
        if (cores > 1)
            threadDepth = (int)Math.Ceiling(Math.Log(cores, 2));
        (double sum, Int64 mask) = KnapsackAsync(0, 0, 0x0);
        int item_count = Convert.ToString(mask, 2).Count(c => c == '1');

        Console.WriteLine($"Sum: {sum:0.000000000} / {limit}");
        Console.WriteLine($"Used items: {item_count} / {items.Length}");
        for (int i = items.Length-1; i >= 0; i--)
        {
            if ((mask & 0x1) != 0)
                Console.WriteLine($"{items[i]:0.000000000}");
            mask >>= 1;
        }
        Console.WriteLine();
    }
}
