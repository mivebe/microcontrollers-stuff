#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

//#define ime"gosho"

int main()
{
/*	char g='G';                //char za edna bukva se pishe s edinichni kavichki i nqma nujda ot opredelenie na broq simvoli
	printf("asbdjfha %c\n",g);
*/	
/*	char ime[]="gosho";       //neopredelen broi simvoli
	printf("%s\n",ime);
	strcpy(ime,"ivan");       //strcpy za promqna na stoinost na string
	printf("%s\n",ime);
*/

char ime [6]="gosho";         // broq simvoli vinagi +1 poveche ot nujnoto
printf("%s\n",ime);
strcpy (ime,"ivan");
printf("%s\n",ime);



	system("pause");
	return 0;
}
