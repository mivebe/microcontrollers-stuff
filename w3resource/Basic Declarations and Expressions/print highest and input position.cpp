#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int chisla[5],i,j,highest=0,pos=0;
	printf("Vuvedete 5 chisla\n");
	for(i=0;i<5;i++){
		printf("Vuvedete chislo %d\n",i+1);
		scanf("%d",&chisla[i]);
		if(chisla[i]>highest){
			highest=chisla[i];
			pos=i+1;
		}
	}
	printf("Highest number is %d input on position %d",highest,pos);
	switch(pos){
		case 1:printf("(first)\n");break;
		case 2:printf("(secound)\n");break;
		case 3:printf("(third)\n");break;
		case 4:printf("(fourth)\n");break;
		case 5:printf("(fifth)\n");break;
	}
	return 0;
}
