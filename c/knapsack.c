#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MULTITHREADING 1

char *filename = "input"; // may be overriden with an argument
int thread_depth = 0; // num of threads = 2^branch_depth
int64_t n = 0; // number of items
double limit; // knapsack size
double items[64];

// synchronous recursive routine
double knapsack(const int64_t depth, double sum, int64_t * const mask)
{
    if (depth >= n)
        return sum;

    *mask <<= 1;
    int64_t mask_b = *mask | 0x1;
    double a = knapsack(depth+1, sum, mask); // try without element #depth

    sum += items[depth];
    if (sum > limit) // pruning
        return a;
    double b = knapsack(depth+1, sum, &mask_b); // try with element #depth

    if (a > b)
        return a;
    *mask = mask_b;
    return b;
}

// thread parameter
struct param {
    int64_t depth;
    double sum;
    int64_t mask;
};

// asynchronous recursive routine
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

    // branch one step deeper
    param->depth++;
    param->mask <<= 1;
    struct param param_b = {
        param->depth,
        param->sum + items[param->depth-1],
        param->mask | 0x1,
    };
    pthread_t thread;
    pthread_create(&thread, NULL, &knapsack_parallel, param); // try without element #depth

    if (param_b.sum <= limit) // pruning
        knapsack_parallel(&param_b); // try with element #depth
    pthread_join(thread, NULL);
    param->depth--;

    if (param->sum < param_b.sum && param_b.sum <= limit) {
        param->sum = param_b.sum;
        param->mask = param_b.mask;
    }
    return NULL;
}

// assists qsorting
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
    if (file == NULL) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filename, strerror(errno));
        return errno;
    }
    fscanf(file, "%lf", &limit);
    while (fscanf(file, "%lf", &items[n]) > 0)
        n++;
    fclose(file);
    struct timespec time_start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

    // Keeping the bigger items at the end allows pruning
    // at the later stages, thus balancing the tree traversal
    qsort(items, n, sizeof(double), compare);
    struct param param = { 0, 0, 0x0 };
    #if MULTITHREADING == 1
        int cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (cores > 1) // the neares upper power of 2 allows to utilize all cores
            thread_depth = 8*sizeof(cores) - __builtin_clz(cores-1);
        knapsack_parallel(&param);
    #else
        #warning single-threaded
        param.sum = knapsack(0, 0, &param.mask);
    #endif

    struct timespec time_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);
    double time = 1e-9 * (time_end.tv_nsec - time_start.tv_nsec) + (time_end.tv_sec - time_start.tv_sec);
    printf("time: %.3f s\n", time);
    printf("elements: %d / %ld\n", __builtin_popcountl(param.mask), n);
    printf("sum: %.9f / %g\n", param.sum, limit);
    while (n--) {
        if (param.mask & 0x1)
            printf("%.9f  ", items[n]);
        param.mask >>= 1;
    }
    puts("");
    return 0;
}
