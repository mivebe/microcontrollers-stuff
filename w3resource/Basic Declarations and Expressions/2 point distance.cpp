#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int x1,y1,x2,y2;
	float distance;
	printf("Vuvedete koordinatite na tochka 1\n");
	scanf("%d %d",&x1,&y1);
	printf("Vuvedete koordinatite na tochka 2\n");
	scanf("%d %d",&x2,&y2);
	distance=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
	printf("Raztoqnieto mejdu dvete tochki e %f\n",distance);
	system("pause");
	return 0;
}
