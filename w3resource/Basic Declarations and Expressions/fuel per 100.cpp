#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	float fuel,distance,lper100; 
	printf("Kolko sa izminatite kilometri?\n");
	scanf("%f",&distance);
	printf("Kolko e izgorenoto gorivo(litri)?\n");
	scanf("%f",&fuel);
	lper100=fuel/(distance/100);
	printf("Prevoznoto sredstvo gori %2f na 100km.\n",lper100);
	system("pause");
	return 0;
}
