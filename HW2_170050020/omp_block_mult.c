#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <mpi.h>

// using namespace std;

#define N 100

int main()
{
	int maxThreads;
	double start, end;
	// int N = 800;
	static float A[N][N], B[N][N], C[N][N]={0};
	
	size_t matSize = N;
	
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			A[i][j]=rand()/(float)RAND_MAX;//range 0 to 1 
			B[i][j]=rand()/(float)RAND_MAX;
		}
	}
	


	start = MPI_Wtime();
	maxThreads = omp_get_max_threads();

	int b = 25;
	// printf("Maximum Number of threads available:\t%d\n",maxThreads);

	#pragma omp parallel for shared (A,B,C,b)


	for(int ii=0;ii<N;ii+=b){
		for(int jj=0;jj<N;jj+=b){
			for(int kk=0;kk<N;kk+=b){
				for(int i=ii;i<=((ii+b-1)>N?N:ii+b-1);i++){
					for(int j =jj;j<=((jj+b-1)>N?N:jj+b-1);j++){
						for(int k=kk;k<=((kk+b-1)>N?N:kk+b-1);k++){
							C[i][j] += A[i][k] * B[k][j];
						}
					}
				}
			}
		}
	}
	for(int i=0;i<N;i++){
		for(int j=0;j<N;j++){
			if(i>j){
				C[i][j]=0;
			}
		}
	}
	end = MPI_Wtime();
	printf("time taken:\t%f\n",end-start);
}