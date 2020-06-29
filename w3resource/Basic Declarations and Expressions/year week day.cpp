#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int d,w,y;
	
	printf("Vuvedete broi dni :\n");
	scanf("%d",&d);
	//d=1562;
	y=d/365;
	w=(d%365)/7;
	d=d-((y*365)+(w*7));
	printf("Tova sa %d godini, %d sedmici i %d dni\n",y,w,d);
	system("pause");
	return 0;
}
