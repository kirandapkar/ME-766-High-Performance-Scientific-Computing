#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <stdlib.h>
int main()
{
	int n=1000000000;
	int a[]={2,4,6,8};
	double pi = 3.141592;
	printf("Sample size =\t%d\n",n);
	printf("\n#Threads\t1st run\t\t2nd run\t\t3rd run\t\t4th run\t\t5th run\t\tAvg. Time\n");
	for(int i=0;i<4;i++){
		double b[5];
		for(int k=0;k<5;k++){
			omp_set_num_threads(a[i]);
			double stime,etime,proc_time;
			stime = omp_get_wtime();
			
			int insideCurve=0;
			double area=0,rand_x,rand_y;
			srand(time(NULL));

			#pragma omp parallel for private(rand_x,rand_y)
			for(int j=1;j<a[i];j++)
			{
				rand_x = (double)rand()/(double)RAND_MAX*pi/2;
				rand_y = (double)rand()/(double)RAND_MAX;
				if(cos(rand_x)>=rand_y)
				{
					#pragma omp atomic update
					insideCurve++;
				}
			}
			area = ((double)insideCurve/(double)a[i])*pi;
			etime =omp_get_wtime();
			proc_time = etime-stime;
			b[k]=proc_time;
			
		}
		printf("%d\t\t%f\t%f\t%f\t%f\t%f\t%f\n", a[i],b[0],b[1],b[2],b[3],b[4],(b[0]+b[1]+b[2]+b[3]+b[4])/5);
	}
		
}