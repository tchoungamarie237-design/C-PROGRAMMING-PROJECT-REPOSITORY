#include<stdio.h>
#include<string.h>

typedef enum{
	LE,
	LA
}side;

typedef struct{
	int id;
	char name[100];
	int age;
	char social_class[50];
	side side;
}Person;

typedef struct category{
	int id;
	char code[50];
	Person guest[4];
	struct category *next;
}category;

Person create_person(){
	Person P;
	printf("\n- - - Guest Input - - -\n");
	printf("ENter an ID\n");
	scanf("%d",&P.id);
	getchar();
	
	printf("\n- - - Guest Name - - -\n");
	printf("Enter a name\n");
	fgets(P.name,sizeof(P.name),stdin);
	 
	 
	printf("\n- - - Guest AGE - - -\n");
	printf("ENter your Age\n");
	scanf("%d",&P.age);
	getchar();
	
	printf("\n- - - Guest Social Class - - -\n");
	printf("Enter your Social Class\n");
	fgets(P.social_class,sizeof(P.social_class),stdin);
	
	printf("\n - - -Guest side- - -\n");
	printf("Enter 0 for LE\n");
	printf("Enter 1 for LA\n");
	scanf("%d",&P.side);
	return P;
		
}

void display_Person(Person P)
{
	int side_choice;
	printf("\n- - - Guest details- - -\n");
	printf("ID:%d\t | Age:%d\t |",P.id,P.age);
	
	if(P.side==0)
	{
		printf("side 'LE'\t");
	}
	else
	{
		printf("side 'LA'\t");
	}
	printf("| Name:%s | Social_class:%s ",P.name,P.social_class);
}


int main() {
    int choice;
    int running = 1; 
    Person current_guest; 

    while (running) {
        printf("\n======= WIGMS SYSTEM =======\n");
        printf("1. Register a Person\n");
        printf("2. Display Last Registered\n");
        printf("0. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Error: Enter a number.\n");
          //  while(getchar() != '\n'); 
            continue;
        }

        switch (choice) {
            case 1:
                current_guest = create_person();
                break;
            case 2:
                display_Person(current_guest);
                break;
            case 0:
                printf("OFF...\n");
                running = 0;
                break;
            default:
                printf("Invalid option.\n");
        }
    }

    return 0;
}