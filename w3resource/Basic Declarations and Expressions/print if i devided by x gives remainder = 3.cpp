#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int i,x,j=0;
	printf("Vuvedete chislo\n");
	scanf("%d",&x);
	printf("Chislata ot 1 do 500 delqshti se na %d s ostatuk 3 sa:\n",x);
	for(i=0;i<=500;i++){
		if((i%x)==3){
			printf("%d\n",i);
			j++;
		}
	}
	printf("Chislata sa %d na broi\n",j);
	return 0;
}
