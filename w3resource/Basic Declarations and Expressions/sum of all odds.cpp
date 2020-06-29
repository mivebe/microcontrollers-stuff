#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int chisla[5],sum=0,i;
	printf("Vuvedete 5 na broi chisla\n");
	for(i=0;i<5;i++){
		printf("Vuvedete chislo %d\n",i+1);
		scanf("%d",&chisla[i]);
		if((chisla[i]%2)==1){
			sum+=chisla[i];
		}
	}
	printf("Sbora na nechetnite chisla e %d\n",sum);
	return 0;
}
