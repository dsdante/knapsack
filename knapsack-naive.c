#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* filename = "input";  // May be overridden with an argument.
u_int8_t n = 0;  // Number of items
double limit;  // Knapsack size
double items[64];

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

    double sum = 0;
    u_int64_t mask = 0x0;
    for (u_int64_t mask_i = 0; mask_i < 0x1U << n; mask_i++) {
        double sum_i = 0;
        for (u_int64_t i = 0; i < n; i++)
            if ((mask_i >> i) & 0x1U)
                sum_i += items[i];
        if (sum < sum_i && sum_i <= limit) {
            sum = sum_i;
            mask = mask_i;
        }
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
