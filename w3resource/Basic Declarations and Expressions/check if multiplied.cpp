#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int x,y;
	printf("Vuvedete 2 chisla.\n");
	scanf("%d %d",&x,&y);
	if((x>y)&&((x%y)==0)){
	printf("Multiplied\n");
	}else if((x<y)&&((y%x)==0)){
		printf("Multiplied\n");
	}else{
		printf("Not multiplied\n");
	}
	system("pause");
	return 0;
}
