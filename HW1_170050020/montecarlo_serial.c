#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <stdlib.h>
int main()
{
	int n=10;
	int a[]={5,10,50,100,500,1000,10000,100000,1000000,5000000};
	double pi = 3.141592;
	printf("Sample Size\tArea\t\tAbsolute Error\tAvg. Time\n");
	for(int i=0;i<n;i++)
	{
		clock_t t;
		t = clock();
		int insideCurve=0;
		double area=0,rand_x,rand_y;
		srand(time(NULL));
		for(int j=1;j<a[i];j++)
		{
			rand_x = (double)rand()/(double)RAND_MAX*pi/2;
			rand_y = (double)rand()/(double)RAND_MAX;
			if(cos(rand_x)>=rand_y)
			{
				insideCurve++;
			}
		}
		area = ((double)insideCurve/(double)a[i])*pi;
		t = clock() - t;
		printf("%d\t\t%f\t%f\t%f\n",a[i],area,fabs(2-area),((double)t)/CLOCKS_PER_SEC);
	}
}