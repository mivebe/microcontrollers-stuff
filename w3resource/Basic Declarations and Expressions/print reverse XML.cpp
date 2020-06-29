#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	/*char x='X',m='M',l='L';
	printf("%c%c%c\n",l,m,x);
	*/
	int i,x=3,m=2,l=1;
	for (i=0;i<4;i++)
	{
		if (i==1)
		{
			printf("L");
		}
		else if (i==2)
		{
			printf("M");
		}
		else if (i==3)
		{
			printf("X\n");
		}
		else
		{
		}
	}
	system("pause");
	return 0;
}
