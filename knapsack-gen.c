#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int n = 0;
    if (argc >= 2)
        n = strtol(argv[1], NULL, 10);
    if (n < 1)
        n = 30;

    double sum = 0;
    srand(time(NULL));
    FILE *file = fopen("input", "w");
    fprintf(file, "%d\n", 3*n);
    for (int i = 0; i < n; i++) {
        double x = 10.0 * rand() / RAND_MAX;
        sum += x;
        fprintf(file, "%.9f\n", x);
        printf("%.9f  ", x);
    }
    fclose(file);
    printf("\nLimit: %d / %.9f\n", 3*n, sum);
    return 0;
}
