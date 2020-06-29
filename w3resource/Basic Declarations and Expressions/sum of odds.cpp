#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int chisla[6],i,sum=0;
	printf("Vuvedete 5 chisla.\n");
	for(i=1;i<6;i++){
		printf("Vuvedete chislo %d.\n",i);
		scanf("%d",&chisla[i]);
		if ((chisla[i]%2)==1)
		{
			sum=sum+chisla[i];
		}
	}
	printf("Sum of odd numbers is %d .\n",sum);
	system("pause");
	return 0;
}
