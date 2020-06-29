#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct laina{
	char ime[30];
	int broi;
	float ocenka;
};

main(){
	struct laina Lainence;
	
	FILE *fp = fopen("C:\\Users\\MIVEBE_1\\""Desktop\\konche.txt","wb");
	     if (fp==NULL){
	     	return 1;
		 }
		 fread(&Lainence,sizeof(struct laina),1,fp);
	    printf("%s\t %d\t %f\t",&Lainence.ime,&Lainence.broi,&Lainence.ocenka);
	    fclose(fp);
	    return 0;
	}
		
