#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PARALLELIZE

const char* filename = "input";  // May be overridden with an argument.
u_int8_t n = 0;  // Number of items
double limit;  // Knapsack size
double items[64];

// Knapsack calculation routine
//   depth: current recursion depth, i.e. the number of the considered items
//   sum: the sum of the taken items among considered
//   mask: the bit mask of the taken items
//   returns the best sum for this branch of recursion
double knapsack(u_int8_t depth, double sum, u_int64_t* mask)
{
    if (depth == n)
        return sum;

    double sum_b = sum + items[depth];
    if (sum_b > limit) {  // Recursion pruning
        *mask <<= (u_int8_t)(n - depth);
        return sum;
    }

    *mask <<= 1U;
    u_int64_t mask_b = *mask | 0x1U;

    sum   = knapsack(depth+1, sum, mask);
    sum_b = knapsack(depth+1, sum_b, &mask_b);

    if (sum_b > sum) {
        sum = sum_b;
        *mask = mask_b;
    }

    return sum;
}

#ifdef PARALLELIZE
u_int8_t thread_depth = 0;  // Threading recursion depth; threads = 2^thread_depth

// Multi-threading version of the knapsack calculation routine
//   depth: current recursion depth, i.e. the number of the considered items
//   sum: the sum of the taken items among considered
//   mask: the bit mask of the taken items
//   returns the best sum for this branch of recursion
double knapsack_parallel(u_int8_t depth, double sum, u_int64_t* mask)
{
    if (depth == n)
        return sum;

    if (depth == thread_depth)  // No more thread spawning.
        return knapsack(depth, sum, mask);

    double sum_b = sum + items[depth];
    if (sum_b > limit) {  // Recursion pruning
        *mask <<= (u_int8_t)(n - depth);
        return sum;
    }

    *mask <<= 1U;
    u_int64_t mask_b = *mask | 0x1U;

    // Spawn A, run B synchronously.
    #pragma omp task default(none) shared(depth, sum, mask)
    sum   = knapsack_parallel(depth+1, sum, mask);
    sum_b = knapsack_parallel(depth+1, sum_b, &mask_b);
    #pragma omp taskwait

    if (sum_b > sum) {
        sum = sum_b;
        *mask = mask_b;
    }

    return sum;
}
#endif // PARALLELIZE

// Qsort comparator
int compare(const void* a, const void* b)
{
    if (*(double*)a < *(double*)b) return -1;
    if (*(double*)a > *(double*)b) return 1;
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc >= 2)
        filename = argv[1];
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        return errno;
    }
    printf("Running '%s'...\n", filename);
    if (fscanf(file, "%lf", &limit) == 0) {
        fprintf(stderr, "Cannot read '%s': invalid format\n", filename);
        fclose(file);
        return -1;
    }
    while (n < 64 && fscanf(file, "%lf", &items[n]) > 0)
        n++;
    fclose(file);

    double sum = 0.0;
    u_int64_t mask = 0x0;

    if (n > 0)
    {
        // Keeping bigger items at the end allows pruning at the later stages,
        // thus balancing the recursion tree
        qsort(items, n, sizeof(items[0]), compare);

        #ifdef PARALLELIZE
        long cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (cores > 1)  // Branching recursion depth = ceil(log2(cores))
            thread_depth = 8 * sizeof(cores) - __builtin_clzl(cores-1);
        #pragma omp parallel default(none) shared(sum, mask)
        #pragma omp single
        sum = knapsack_parallel(0, 0.0, &mask);
        #else
        #warning Single-threaded
        sum = knapsack(0, 0.0, &mask);
        #endif
    }

    printf("Sum: %.9f / %g\n", sum, limit);
    printf("Used items: %d / %u\n", __builtin_popcountl(mask), n);
    while (n--) {
        if (mask & 0x1U)
            printf("%.9f\n", items[n]);
        mask >>= 1U;
    }
    return 0;
}
