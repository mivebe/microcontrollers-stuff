#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int i;
	printf("Kvadrat na chetnite chisla ot 0 do 10\n");
	for(i=1;i<=10;i++){
		if((i%2)==0){
			printf("%dx%d=%d\n",i,i,i*i);
		}
	}
	return 0;
}
