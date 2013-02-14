#include "syscall.h"


void matMultTC(int instance)
{
	int index;
	for(index = 1 ;index <= instance; index++)
	{
		Exec("../test/matmult", 15);
		
	}
}

void sortTC(int instance)
{
	int index;
	for(index = 1 ;index <= instance; index++)
	{
		Exec("../test/sort", 12);
		
	}
}


void main()
{
	int userInput = 0;
	char buf[3];
	
    /* Output statements for test case menu */
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n");
	Print_Stmt("\n\t\t			     PROJECT 3		  GROUP - 58\n\n");
	Print_Stmt("\t\t\t		     Virtual Memory and Basic Networking\n");
	Print_Stmt("\t\t\t		      >>>>>>>> TEST CASES PART 1 & 2 <<<<<<<<\n");
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n\n");
	
	Print_Stmt(" 1 > Single Exec MatMult\n");
	Print_Stmt(" 2 > Two Exec MatMult\n");
	Print_Stmt(" 3 > Single Exec Sort\n");
	Print_Stmt(" 4 > Two Exec Sort\n");
	Print_Stmt(" 5 > Two Fork MatMult\n");
	Print_Stmt(" 6 > Two Fork Sort\n");
	Print_Stmt("\n\nEnter the index of the testcase you want to run here ->");
	userInput = Scan("%d"); 
	
 	switch(userInput)
	{	
		case 1:	
				matMultTC(1);
				break;
		case 2:		matMultTC(2);
				break;
		case 3:		sortTC(1);
				break;
		case 4:		sortTC(2);
				break;
		case 5: Exec("../test/forkthread", 18);
				break;
		case 6: Exec("../test/forkthreadSort", 22);
				break;				
		default: Print_Stmt("\n Invalid option entered by the user !!\n\n");
				
	}
}