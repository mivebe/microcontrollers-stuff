#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>
int main()
{
	int i,j;
	char ime[40],ime1[40],prezime[40],familiq[40];
	printf("Napishete vashite imena?\n");
	gets(ime);
	for(i=0;ime[i]!=' ';i++)
		ime1[i]=ime[i];
	for(i,j=0;ime[i]!=' ';i++){
		ime1[i]=prezime[j];
		j++;
	}
	for(i,j=0;ime[i]!='\0';i++){
		ime1[i]=familiq[j];
		j++;
	}
	printf("vasheto ime e ");
	puts(ime1);
	printf("vasheto prezime e");
	puts(prezime);
	printf("Vashata familiq e");
	puts(familiq);
	getch();
	return 0;
}
