#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int salary,workhours,workdays,id,perhour;
	
	printf("What is employee's ID?\n");
	scanf("%d",&id);
	while(1)
	{
		printf("State ammount of employee #%d's monthly workdays.\n",id);
		scanf("%d",&workdays);
		if(workdays<=0||workdays>31)
		{
			printf("Invalid number!!!\n");
		}
		else
		{
			break;
		}
	}
	while(1)
	{
		printf("State employee #%d's workhours per day.\n",id);
		scanf("%d",&workhours);
		if(workhours<=0||workhours>24)
		{
			printf("Invalid number!!!\n");
		}
		else
		{
			break;
		}
	}
	printf("What is employee #%d's monthly salary?\n",id);
	scanf("%d",&salary);
	perhour=(salary/(workhours*workdays));
	printf("Employee #%d receives $%d per hour.",id,perhour);
	system("pause");
	return 0;
}
