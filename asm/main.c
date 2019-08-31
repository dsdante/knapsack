#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PARALLELIZE

const char *filename = "input";  // May be overridden with an argument.
u_int64_t n = 0;  // Number of items
double limit;  // Knapsack size
double items[64];
size_t thread_depth = 0;  // Threading recursion depth; threads = 2^thread_depth

// Recursive knapsack calculation routine
//   depth: the number of the considered items (depth of recursion)
//   sum: the sum of the taken items among considered
//   mask: bit mask of the taken items
//   returns the best sum for this branch and updates the mask
double knapsack(u_int64_t depth, double sum, u_int64_t * mask)
{
    if (n == 0) return sum;
    double sum_b = 0;
    int64_t mask_b = 0x0;
    asm (
    "sub   $64, %%rsp;"  // avoid overwriting stack frame
    "call  start;"       // allow the last ret
    "add   $64, %%rsp;"
    "jmp   end;"

    // fastcall
    //   in int depth
    //   in/out double sum
    //   in/out int mask
    "start:"
    "movsd %[sum], %[sum_b];"   // sum_b = sum;
    "shl   $1, %[mask];"        // mask <<= 1;
    "mov   %[mask], %[mask_b];" // mask_b = mask | 0x1;
    "or    $0x1, %[mask_b];"
    "addsd (%[items], %[depth], 8), %[sum_b];" // sum_b += items[depth];

    "inc   %[depth];"           // depth++;
    "cmp   %[n], %[depth];"     // if (depth < n)
    "jl    recursive_calls;"    //     go deeper;
    "comisd %[limit], %[sum_b];"// if sum_b > limit
    "ja    return;"             //     return a;
    "jmp   select_better;"      // else select which one is better;

    "recursive_calls:"
    "sub   $16, %%rsp;"
    "movsd %[sum_b], 8(%%rsp);" // push sum_b
    "mov   %[mask_b], (%%rsp);" // push mask_b
    "call  start;"              // a = routine(depth, sum, mask);  // try without items[depth]
    "mov   (%%rsp), %[mask_b];" // pop mask_b
    "movsd 8(%%rsp), %[sum_b];" // pop sum_b
    "add   $16, %%rsp;"

    "comisd %[limit], %[sum_b];"// if sum_b > limit  // pruning
    "ja    return;"             //     return a

    "xchg  %[mask], %[mask_b];" // swap a and b; we don't care for the order
    "xorpd %[sum], %[sum_b];"
    "xorpd %[sum_b], %[sum];"
    "xorpd %[sum], %[sum_b];"
    "sub   $16, %%rsp;"
    "movsd %[sum_b], 8(%%rsp);" // push sum_b
    "mov   %[mask_b], (%%rsp);" // push mask_b
    "call  start;"              // a = routine(depth, sum, mask);  // try with items[depth]
    "mov   (%%rsp), %[mask_b];" // pop mask_b
    "movsd 8(%%rsp), %[sum_b];" // pop sum_b
    "add   $16, %%rsp;"

    "select_better:"
    "comisd %[sum_b], %[sum];"  // if (sum > sum_b)
    "ja    return;"             //     return a;
    "movsd %[sum_b], %[sum];"   // else
    "mov   %[mask_b], %[mask];" //     return b;

    "return:"
    "dec   %[depth];"           // depth--;
    "ret;"                      // return;
    "end:"
    : // output
    [sum] "+&rx" (sum),
    [mask] "+&r" (*mask)
    : // input
    [n] "r" (n),
    [limit] "rx" (limit),
    [items] "rN" (items),
    // locals
    [depth] "rN" (depth),
    [sum_b] "rx" (sum_b),
    [mask_b] "r" (mask_b)
    );
    return sum;
}

// Thread parameter
struct param {
    u_int64_t depth;
    double sum;
    u_int64_t mask;
};

// Multi-threading version of the routine
void* knapsack_parallel(void *_param)
{
    struct param *param = _param;
    if (param->depth == n)
        return NULL;

    if (param->depth == thread_depth) {
        // No more branching
        param->sum = knapsack(param->depth, param->sum, &param->mask);
        return NULL;
    }

    double sum_b = param->sum + items[param->depth];
    if (sum_b > limit) {
        // Recursion pruning
        param->mask <<= n - param->depth;
        return NULL;
    }

    param->depth++;
    param->mask <<= 1U;
    struct param param_b = {
            param->depth,
            sum_b,
            param->mask | 0x1U,
    };

    // Spawn A, run B synchronously.
    #pragma omp task default(none) shared(param)
    knapsack_parallel(param);
    knapsack_parallel(&param_b);
    #pragma omp taskwait

    if (param->sum < param_b.sum) {
        param->sum = param_b.sum;
        param->mask = param_b.mask;
    }
    return NULL;
}

// Qsort comparator
int compare(const void *a, const void *b)
{
    if (*(double*)a < *(double*)b) return -1;
    if (*(double*)a > *(double*)b) return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc >= 2)
        filename = argv[1];
    FILE *file = fopen(filename, "r");
    printf("Running '%s'...\n", filename);
    if (file == NULL) {
        fprintf(stderr, "Cannot open '%s': %s\n", filename, strerror(errno));
        return errno;
    }
    if (fscanf(file, "%lf", &limit) == 0) {
        fprintf(stderr, "Cannot read '%s': invalid format\n", filename);
        fclose(file);
        return -1;
    }
    while (n < 64 && fscanf(file, "%lf", &items[n]) > 0)
        n++;
    fclose(file);

    // Keeping bigger items at the end allows pruning at the later stages,
    // thus balancing the recursion tree
    qsort(items, n, sizeof(double), compare);
    struct param param = { 0, 0, 0x0 };
    #ifdef PARALLELIZE
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores > 1) // branching recursion depth = ceil(log2(cores))
        thread_depth = 8 * sizeof(cores) - __builtin_clzl(cores-1);
    #endif
    #pragma omp parallel default(none) shared(param)
    #pragma omp single
    knapsack_parallel(&param);

    printf("Sum: %.9f / %g\n", param.sum, limit);
    printf("Used items: %d / %ld\n", __builtin_popcountl(param.mask), n);
    while (n--) {
        if (param.mask & 0x1U)
            printf("%.9f\n", items[n]);
        param.mask >>= 1U;
    }
    return 0;
}
