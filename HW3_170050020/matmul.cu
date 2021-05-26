#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define BLOCKSIZE 16

__global__ void Cuda_Mult(int *d_a, int *d_b, int *d_res, int n){
    // dot product of two matrices 
    __shared__ int T1[BLOCKSIZE][BLOCKSIZE];
    __shared__ int T2[BLOCKSIZE][BLOCKSIZE];

    int R = blockIdx.y * BLOCKSIZE + threadIdx.y,C = blockIdx.x * BLOCKSIZE + threadIdx.x;
    int idx,Temp = 0;

    for (int i = 0; i < gridDim.x; ++i){
        idx = R * n + i * BLOCKSIZE + threadIdx.x;
        if(idx >= n*n){
            T1[threadIdx.y][threadIdx.x] = 0;
        }
        else{
            T1[threadIdx.y][threadIdx.x] = d_a[idx];
        }

        idx = (i * BLOCKSIZE + threadIdx.y) * n + C;
        if(idx >= n*n){
            T2[threadIdx.y][threadIdx.x] = 0;
        }  
        else{
            T2[threadIdx.y][threadIdx.x] = d_b[idx];
        }
        __syncthreads();

        for (int k = 0; k < BLOCKSIZE; ++k) {
            Temp += T1[threadIdx.y][k] * T2[k][threadIdx.x];
        }
        __syncthreads();
    }
    if(R < n && C < n){
        d_res[R * n + C] = Temp;
    }
}



void CPU_Mult(int *a, int *b, int *res, int n){
    // function to obtain dot product of two matrices
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int ans = 0.0;
            for (int h = 0; h < n; h++) {
                ans += a[i * n + h] * b[h * n + j];
            }
            res[i * n + j] = ans;
        }
    }
}


int main(int argc, char* argv[]){
    
    int option = 1;
    if(argc>1){   
        if(*argv[2] == 'S'){
            option = 0;
        }
    }
    //Fixed seed for illustration
    int N = 100;
    srand(1234);

    //allocate memory
    int *A, *B, *C, *new_C;
    cudaMallocHost((void **) &A, sizeof(int)*N*N);
    cudaMallocHost((void **) &B, sizeof(int)*N*N);
    cudaMallocHost((void **) &C, sizeof(int)*N*N);
    
    // randomly initialize matrix A
    for (int i = 0; i < N; ++i){
		for (int j = 0; j < N; ++j){
			A[i*N + j] = (double)rand()/(double)(RAND_MAX/N);
		}
	}

    // randomly initialize matrix B
    for (int i = 0; i < N; ++i){
		for (int j = 0; j < N; ++j){
            B[i*N + j] = (double)rand()/(double)(RAND_MAX/N);
        }
    } 
    float gpu_elapsed_time;
    float cpu_elapsed_time;

    //event to calculate execution time
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);


    if(option==1){
    	printf("To run serial mult on cpu use: %s -o S\n",argv[0]);

        //allocate space
		int *dev_A, *dev_B, *dev_C;
		cudaMalloc((void **) &dev_A, sizeof(int)*N*N);
		cudaMalloc((void **) &dev_B, sizeof(int)*N*N);
		cudaMalloc((void **) &dev_C, sizeof(int)*N*N);

		cudaEventRecord(start, 0);

        // copy matrix A and B from host to device mem
		cudaMemcpy(dev_A, A, sizeof(int)*N*N, cudaMemcpyHostToDevice);
		cudaMemcpy(dev_B, B, sizeof(int)*N*N, cudaMemcpyHostToDevice);

		unsigned int grid_Rs = (N + BLOCKSIZE - 1) / BLOCKSIZE;
		unsigned int gridev_Cs = (N + BLOCKSIZE - 1) / BLOCKSIZE;
		dim3 dimGrid(gridev_Cs, grid_Rs);
		dim3 dimBlock(BLOCKSIZE, BLOCKSIZE);

        //Launch Kernal
		Cuda_Mult<<<dimGrid, dimBlock>>>(dev_A, dev_B, dev_C, N);    

        // Transefr results from device to host 
		cudaMemcpy(C, dev_C, sizeof(int)*N*N, cudaMemcpyDeviceToHost);

        //stop counting time
		cudaEventRecord(stop, 0);
		cudaEventSynchronize(stop);

        //time for cuda evaluation
		cudaEventElapsedTime(&gpu_elapsed_time, start, stop);
		printf("Time for mat mult of %dx%d . %dx%d on GPU: %f ms.\n\n", N, N, N, N, gpu_elapsed_time);

		cudaFree(dev_A);
	    cudaFree(dev_B);
	    cudaFree(dev_C);
    }
    else{
    	cudaMallocHost((void **) &new_C, sizeof(int)*N*N);

    	cudaEventRecord(start, 0);
        //call normal multiplication by CPu. 
	    CPU_Mult(A, B, new_C, N);
	    
	    cudaEventRecord(stop, 0);
	    cudaEventSynchronize(stop);
	    
        //time for cpu evaluation
	    cudaEventElapsedTime(&cpu_elapsed_time, start, stop);
	    printf("Time elapsed on matrix multiplication of %dx%d . %dx%d on CPU: %f ms.\n\n", N, N, N, N, cpu_elapsed_time);

	    cudaFreeHost(new_C);
    }
    
    cudaFreeHost(A);
    cudaFreeHost(B);
    cudaFreeHost(C);
    
    return 0;
}