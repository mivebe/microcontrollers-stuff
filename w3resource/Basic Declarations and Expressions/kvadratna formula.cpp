#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	float a,b,c,D;
	printf("Vuvedete koeficientite na kvadratnoto uravnenie.\n");
	scanf("%f %f %f",&a,&b,&c);
	D=(b*b)-(4*(a)*(c));
	if(D>0&&a!=0){
	printf("koren1=%f\n",((-b)+sqrt(D))/(2*a));
	printf("koren2=%f\n",((-b)-sqrt(D))/(2*a));
	}else{
		printf("Nqma resenie.\n");
	}
	system("pause");
	return 0;
}
