#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	float hui;
	printf("Kolko ti e golqm huq?\n");
	scanf("%f",&hui);
	if(hui>0&&hui<=3){
		printf("Tva nee hui!\n");
	}else if(hui>3&&hui<=5){
		printf("Ti si shiban jaltur!\n");
	}else if(hui>5&&hui<=10){
		printf("Vse e neshto!\n");
	}else if(hui>10&&hui<=14){
		printf("Imash sreden hui!\n");
	}else if(hui>14&&hui<=20){
		printf("Imash golqm hui!\n");
	}else if(hui>20&&hui<=25){
		printf("Basi negura!\n");
	}else{
	printf("E nqq takuv hui!\n");	
	}
	system("pause");
	return 0;
}
