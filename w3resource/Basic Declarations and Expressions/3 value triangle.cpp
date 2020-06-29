#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	float a,b,c,P;
	printf("Vuvedete tri stoinosti.\n");
	scanf("%f %f %f",&a,&b,&c);
	if(a<(b+c)&&b<(a+c)&&c<(a+b)){
		P=a+b+c;
		printf("Ako priemem stoinostite za duljini na stranite na triugalnik");
		printf("to toi shte ima obikolka: %f\n",P);
	}else{
		printf("Ot stoinostite nemoje da see suzdade triugalnik!\n");
	}
	system("pause");
	return 0;
}
