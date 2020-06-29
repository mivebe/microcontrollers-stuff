#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main ()
{
	int q1,q2;
	double w1,w2,result;
	printf("Vuvedete tegloto na predmet #1\n");
	scanf("%lf",&w1);
	printf("Vuvedete kolichestvoto ot predmet #1\n");
	scanf("%d",&q1);
	printf("Vuvedete tegloto na predmet #2\n");
	scanf("%lf",&w2);
	printf("Vuvedete kolichestvoto ot predmet #2\n");
	scanf("%d",&q2);
	result=((w1*q1)+(w2*q2))/(q1+q2);
	printf("Srednoto teglo na predmet e %lf\n",result);
	system("pause");
	return 0;
}
