#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

int main()
{
	int year,month,day;
	char personal[30],fatherly[30],family[30];
	printf("Vuvedete trite si imena.\n");
	scanf(" %s %s %s",&personal,&fatherly,&family);
	printf("Vuvedete vashata data na rajdane.\n");
	scanf(" %d %d %d",&year,&month,&day);

	
	printf(" Zdraveite %s %s %s!\n",personal,fatherly,family);
	printf("Vashata data na rajdane e: %d\\%d\\%d \n",year,month,day);	
	printf("S dumi: %d den, %d mesec, %d godina\n",day,month,year);





system("pause");
	return 0;
}
