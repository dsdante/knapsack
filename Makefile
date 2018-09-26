knapsack-gen: knapsack-gen.c
	gcc $(CFLAGS) -o knapsack-gen knapsack-gen.c

knapsack-naive: knapsack-naive.c
	gcc $(CFLAGS) -o knapsack-naive knapsack-naive.c

all: knapsack-gen knapsack-naive

clean:
	rm -f *.o *.out knapsack-gen knapsack-naive
