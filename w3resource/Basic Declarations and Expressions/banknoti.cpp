#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main()
{
	int suma,broi;
	printf("Kolko pari imash we kaltak?\n");
	scanf("%d",&suma);
	printf("Tova sa:\n");
	broi=(suma/100);
	suma=suma-(broi*100);
	printf("%d banknoti po 100 lv\n",broi);
	broi=(suma/50);
	suma=suma-(broi*50);
	printf("%d banknoti po 50 lv\n",broi);
	broi=(suma/20);
	suma=suma-(broi*20);
	printf("%d banknoti po 20 lv\n",broi);
	broi=(suma/10);
	suma=suma-(broi*10);
	printf("%d banknoti po 10 lv\n",broi);
	broi=(suma/5);
	suma=suma-(broi*5);
	printf("%d banknoti po 5 lv\n",broi);
	broi=(suma/2);
	suma=suma-(broi*2);
	printf("%d banknoti po 2 lv\n",broi);
	broi=(suma/1);
	suma=suma-(broi*1);
	printf("%d banknoti po 1 lv\n",broi);
	system("pause");
	return 0;
}
