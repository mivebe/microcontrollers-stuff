#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main()
{
	/*	
	int a;

	printf ("yo bitch \n");
	printf("fatfinger a number \n");
	scanf("%d", &a);
	printf("you fatfingered %d \n",a);
	system("pause");
	return 0;  
	*/
	
/*	
	int i;
	for (i=1;i<=10;i++)
	printf ("%d\n",i);
*/
/*
	int i=1;

	loop:
		if (i<=10)
		{
			printf("%d\n",i);
			++i;
		
		}
			goto loop;
*/

	int i;
	i=1;
	do
	{
		printf("%d\n",i);
		i++;
	}
		while(i<=10);

	system("pause");
	return 0;	
}

