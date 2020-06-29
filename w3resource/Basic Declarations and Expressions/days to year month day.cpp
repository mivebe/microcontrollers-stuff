#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int days,months,years;
	printf("How many days have passed?\n");
	scanf("%d",&days);
	printf("Tova sa:\n");
	years=days/365;
	printf("%d godini\n",years);
	months=(days-(years*365))/30;
	printf("%d mesec(a)\n",months);
	days=days-((years*365)+(months*30));
	printf("%d den(a)\n",days);
	system("pause");
	return 0;
}
