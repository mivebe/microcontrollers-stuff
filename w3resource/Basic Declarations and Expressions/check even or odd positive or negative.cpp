#include<stdio.h>
#include<stdlib.h>
#include<math.h>

int main(){
	int a;
	printf("Vuvedete chislo\n");
	scanf("%d",&a);
	printf("Vuvedenoto chislo e: ");
	if (a>0&&(a%2)==0){printf("Chetno Polojitelno\n");
	}else if (a>0&&(a%2)==1){printf("Nechetno Polojitelno\n");
	}else if (a<0&&(a%2)==0){printf("Chetno Otricatelno\n");
	}else if (a<0&&(a%2)==1){printf("Nechetno Otricatelno\n");
	}else {printf("0 ne e podhodqshto chislo\n");
	}
	return 0;
}
