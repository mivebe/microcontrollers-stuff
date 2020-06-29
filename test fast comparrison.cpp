#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
               
int main()     
{              
	int age;   
//	printf("How old are you?\n");
//	scanf("%d",&age);
//	printf("You are %d years old and you are %s\n",age,(age>=13)&&(age<=19)?"a teenager":"not a teenager");
	char  *teenAge[30];
	teenAge[30]=(age>=13)&&(age<=19)? "a teenager":"not a teenager");
	printf("How old are you?\n");
	scanf("%d",&age);
                	
	printf("You are %d years old and you are %s\n",age,teenAge);
	system("pause");
	return 0;  
}

