#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int sec,min,hour;
	printf("Kolko sekundi sa izminali?\n");
	scanf("%d",&sec);
	printf("Tova sa:\n");
	hour=sec/3600;
	printf("%d chasa\n",hour);
	min=(sec-(hour*3600))/60;
	printf("%d minuti\n",min);
	sec=sec-((hour*3600)+(min*60));
	printf("%d sekundi\n",sec);
	system ("pause");
	return 0;
}
