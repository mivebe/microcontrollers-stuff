#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int i,j=0,num[5],sum;
	for(i=0;i<5;i++){
	printf("Vuvedete cifra.\n");
	scanf("%d",&num[i]);
	}
	for(j=0;j<5;j++){
		if ((num[j]%2)!=0){
			sum+=num[j];
		}
	}
	printf("Sbora na nechetnite ot vuvedenite chisla e: %d\n",sum);
	system("pause");
	return 0;
}
