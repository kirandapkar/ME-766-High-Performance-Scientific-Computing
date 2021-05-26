#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <mpi.h>

// using namespace std;

#define N 1000

int main()
{
	int maxThreads;
	double start, end;
	// int N = 1000;
	float A[N][N], B[N][N], C[N][N]={0};
	
	size_t matSize = N;
	// int size = N*N;
	float r = 1.0;//rand()/(float)RAND_MAX;
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			A[i][j]=r;//range 0 to 1 
			B[i][j]=r;

		}
	}
	


	start = MPI_Wtime();
	maxThreads = omp_get_max_threads();

	printf("Maximum Number of threads available:\t%d\n",maxThreads);

	#pragma omp parallel for shared (A,B,C)

	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			// C[i][j]=0;
			for(int k=0;k<N;k++){
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}
	end = MPI_Wtime();
	printf("time taken:\t%f\n",end-start);
}