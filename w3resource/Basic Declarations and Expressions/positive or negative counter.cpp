#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int chisla[5],j,z=0,i,p=0,n=0;
	printf("Vuvedete 5 na broi chisla\n");
	for(i=0;i<5;i++){
		printf("Vuvedete chislo\n");
		scanf("%d",&chisla[i]);
	}
	for(j=0;j<5;j++){
		if (chisla[j]>0){
			p++;
		}else if(chisla[j]<0){
			n++;
		}else{
			z++;
		}
	}
	printf("Broq na polojitelnite chisla e %d\n",p);
	printf("Broq na otricatelnite chisla e %d\n",n);
	printf("Vuvedeni sa %d na broi 0\n",z);
	return 0;
}
