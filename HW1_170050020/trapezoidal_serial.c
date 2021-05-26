#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <time.h>
int main()
{
	int n=10;
	int a[]={2,5,10,20,50,100,200,500,1000,10000};
	double pi = 3.141592;
	printf("Sample Size\tArea\t\tAbsolute Error\tAvg. Time\n");
	for(int i=0;i<n;i++)
	{
		clock_t t;
		t = clock();
		double area=0,h=pi/a[i],xi=-pi/2,xj=h+xi;
		int f;

		for(int j=1;j<a[i];j++)
		{
			area = area + (h/2)*(cos(xi)+cos(xj));
			xi+=h;
			xj=xi;
		}
		t = clock() - t;
		printf("%d\t\t%f\t%f\t%f\n",a[i],area,fabs(2-area),((double)t)/CLOCKS_PER_SEC);
	}
}