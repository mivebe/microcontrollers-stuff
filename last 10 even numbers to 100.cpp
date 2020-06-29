#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	/*int i;

	for (i=0;i<=10;i++)
	{
		if ((i % 2) == 0)
		{
			printf("%d\n",i);
		}
	}
	*/
	
	int i;
	for (i=0;i<=100;i++)
	{
		if ((i % 2)==0 && i>=80)
		{
			printf("%d\n",i);
		}
	}
	system("pause");
	return 0;
}
