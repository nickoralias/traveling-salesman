#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

int *dist; //the distance matrix
int num; //number of cities
int *best_path; //best path found
int *my_path; //path being explored
int min_dist; //shortest path distance found
int *my_dist; //dist of current path

void get_input(char filename[])
{
	FILE *fp;
	int i;

	/* read the file, intialize dist */
	fp = fopen(filename, "r");
	if (!fp)
	{
		printf("Cannot open file %s\n", filename);
	 	exit(1);
	}
	num = 0;
	int next = 0;
	while (next != EOF)
	{
		next = fgetc(fp);
		if (next == '\n')
		{
			num++;
		}
	}
	fclose(fp);

	/* allocate memory for dist */
	dist = (int*)malloc(num*num*sizeof(int));

	fp = fopen(filename, "r");
	
	for(i = 0; i < (num * num); i++)
	{
		fscanf(fp, "%d", &dist[i]);
	}
}

void print_path()
{
	int i, tid = omp_get_thread_num();
	printf("My path: ");
	for (i = 0; i < num; i++)
	{
		printf("%d ", my_path[tid * num + i]);
	}
	printf("\n");
}

int add_dist(int *array)
{
	int i, distance = 0, tid = omp_get_thread_num();
	for (i = 0; i < num - 1; i++)
	{
		distance += dist[(array[num * tid + i]) * num + (array[num * tid + i + 1])];
	}
	return distance;
}

void ts(int depth, int tid)
{
	int i;
	my_dist[tid] = add_dist(my_path);		
	if (depth == 1)
	{
		if (my_dist[tid] < min_dist)
		{
			#pragma omp critical
			{
				min_dist = my_dist[tid];
				for (i = 0; i < num; i++)
				{
					best_path[i] = my_path[tid * num + i];
				}
			}
		}	
	}
	else if ((my_dist[tid] <= min_dist) || (depth > num/2))
	{
		tid = omp_get_thread_num();
		for (i = 1; i < depth; i++)
		{
			/* swap cities in location depth and i */
			int tmp = my_path[tid * num + depth - 1];
			my_path[tid * num + depth - 1] = my_path[tid * num + i];
			my_path[tid * num + i] = tmp;

			/* recurse on this permutation */
			ts(depth - 1, tid);

			/* restore the swapped cities */
			tmp = my_path[tid * num + depth - 1];
			my_path[tid * num + depth - 1] = my_path[tid * num + i];
			my_path[tid * num + i] = tmp;
		}
	}
}

void ts_driver(int depth)
{
	int i;
	#pragma omp parallel for num_threads(num - 1)
	for (i = 1; i < depth; i++)
	{
		int tid = omp_get_thread_num();

		/* swap cities in location depth and i */
		int tmp = my_path[tid * num + depth - 1];
		my_path[tid * num + depth - 1] = my_path[tid * num + i];
		my_path[tid * num + i] = tmp;

		/* recurse without nested parallelism */
		ts(depth - 1, tid);
	}
}

void print_input()
{
	printf("Num = %d\n", num);
	int i;
	for(i = 0; i < (num * num); i++)
	{
		printf("%d\n", dist[i]);
	}
}

void print()
{
	int i;
	printf("Best path: ");
	for (i = 0; i < num; i++)
	{
		printf("%d ", best_path[i]);
	}
	printf("\nDistance: %d\n", min_dist);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
 	{
   		printf("Usage: tsm filename\n");
		exit(1);
   	}

 	/* Read the input file and fill the dist data structure above */ 
   	get_input(argv[1]);

   	/* init global data structures */
   	best_path = (int*)malloc(num * sizeof(int));
	my_path = (int*)malloc(num * num * sizeof(int));
	my_dist = (int*)malloc(num * sizeof(int));

	/* initialize the distances and paths of cities */
	int i, j;
	for (i = 0; i < num; i++)
	{
		best_path[i] = i;
		my_dist[i] = 0;
	}
	for (i = 0; i < num; i++)
	{
		for (j = 0; j < num; j++)
		{
			my_path[i * num + j] = j;
		}
	}
	min_dist = add_dist(best_path);

	/* call the branch and bound algorithm */
	ts_driver(num);

	/* print the results */
	print();

	return 0;
}
