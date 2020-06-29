#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int p,q,r,s;
	r>0;s>0;p%2==0;
	printf("Vuvedete dve polojitelnite chisla R i S kakto sledva.\n");
	scanf("%d %d",&r,&s);
	printf("Vuvedete chisloto P - chetno i sluchainoto chislo Q kakto sledva.\n");
	scanf("%d %d",&p,&q);
	if(q>r&&s>p&&r+s>p+q){
	printf("Correct values\n");
	}else{
		printf("Wrong values\n");
	}
	system("pause");
	return 0;
}
