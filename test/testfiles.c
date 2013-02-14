/* testcases.c
 *	Simple program to test the system calls
 */

#include "syscall.h"

int AcquiredLockIndexReleaseTest = -1;
int AcquiredLockIndexAcquireTest = -1;


/***********************************************************************************************/
/*									CREATE LOCK RPC TEST CASE								   */		
/***********************************************************************************************/
void CreateLock_RPC_test() {
	
	int lockIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tCREATE LOCK RPC TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		CREATE LOCK WITH VALID PARAMETERS\n");
	lockIndex = CreateLock("newLock", 8);
	if(lockIndex == -1){
		Print_Stmt("CreateLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("CreateLock_RPC_test : Lock Created with Index %d\n", lockIndex);
	
	/*-----------------------------------------------------------------*/
	/* 					CREATE LOCK WITH INVALID LENGTH				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		CREATE LOCK WITH INVALID LENGTH\n");
	lockIndex = 0;
	lockIndex = CreateLock("newLock2", -1);
	if(lockIndex == -1){
		Print_Stmt("CreateLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("CreateLock_RPC_test : Lock Created with Index %d\n", lockIndex);
	
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK WITH THE SAME NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		CREATE LOCK WITH THE SAME NAME\n");
	lockIndex = 0;
	lockIndex = CreateLock("newLock", 8);
	if(lockIndex == -1){
		Print_Stmt("CreateLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("CreateLock_RPC_test : Lock Created with Index %d\n", lockIndex);
		
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK WITH NO NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		CREATE LOCK WITH NO NAME\n");
	lockIndex = 0;
	lockIndex = CreateLock("",1);
	if(lockIndex == -1){
		Print_Stmt("CreateLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("CreateLock_RPC_test : Lock Created with Index %d\n", lockIndex);	
	
	Exit(0);
}

/***********************************************************************************************/
/*								ACQUIRE LOCK RPC TEST CASE						   			   */	
/***********************************************************************************************/
void ChildThreadAcquireLock_test() {

	int lockIndex = -1;
	
	/*-----------------------------------------------------------------------------*/
	/*	CHILD THREAD TRYING TO ACQUIRE A LOCK ACQUIRED BY PARENT THREAD IN TEST 3  */	
	/*-----------------------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t		CHILD THREAD TRYING TO ACQUIRE A LOCK ACQUIRED BY PARENT THREAD IN TEST 3\n");
	lockIndex = Acquire(AcquiredLockIndexAcquireTest);	
	if(lockIndex == -1){
		Print_1Arg("ChildThreadAcquireLock_test : Child could not Acquire the Lock %d Acquired by parent\n", AcquiredLockIndexReleaseTest);
	}else	
		Print_1Arg("ChildThreadAcquireLock_test : Child Acquired the Lock %d Acquired by parent\n", lockIndex);
	
	Exit(0);
}

void AcquireLock_RPC_test() {

	int createdlockIndex = -1;
	int lockIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tACQUIRE LOCK RPC TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE A LOCK WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : CREATE A LOCK WITH VALID PARAMETERS\n");
	createdlockIndex = CreateLock("AcquireTestLock", 16);
	if(createdlockIndex == -1){
		Print_Stmt("AcquireLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("AcquireLock_RPC_test : Lock Created with Index %d\n", createdlockIndex);
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		ACQUIRE A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = Acquire(createdlockIndex);	
	if(lockIndex == -1){
		Print_Stmt("AcquireLock_RPC_test : Lock was not Acquired\n");
	}else	
		Print_1Arg("AcquireLock_RPC_test : Lock Acquired with Index %d\n", lockIndex);
		
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK WITH INVALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		ACQUIRE A LOCK WITH INVALID LOCK INDEX\n");
	lockIndex = Acquire(-10);	
	if(lockIndex == -1){
		Print_Stmt("AcquireLock_RPC_test : Lock was not Acquired\n");
	}else	
		Print_1Arg("AcquireLock_RPC_test : Lock Acquired with Index %d\n", lockIndex);	
			
		
	/*-----------------------------------------------------------------*/
	/*	ACQUIRE A LOCK WHICH IS ALREADY ACQUIRED BY THE SAME CLIENT	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		ACQUIRE A LOCK WHICH IS ALREADY ACQUIRED BY THE SAME CLIENT\n");
	lockIndex = Acquire(createdlockIndex);	
	AcquiredLockIndexAcquireTest = lockIndex;
		
	if(lockIndex == -1){
		Print_Stmt("AcquireLock_RPC_test : Lock was not Acquired\n");
	}else	
		Print_1Arg("AcquireLock_RPC_test : Lock Acquired with Index %d\n", lockIndex);	
		
	/*-----------------------------------------------------------------*/
	/*	MAKE CHILD THREAD ACQUIRE A LOCK ACQUIRED BY PARENT THREAD	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		MAKE CHILD THREAD ACQUIRE A LOCK ACQUIRED BY PARENT THREAD IN TEST3\n");
	
	Fork(ChildThreadAcquireLock_test);
 	Yield();
	Print_Stmt("\t		PARENT THREAD RELEASING THE LOCK ACQUIRED IN TEST3\n");	
	
	lockIndex = Release(AcquiredLockIndexAcquireTest);	
	if(lockIndex == -1){
		Print_1Arg("AcquireLock_RPC_test : Parent Thread could not release the lock %d\n\n", AcquiredLockIndexAcquireTest);
	}else	
		Print_1Arg("AcquireLock_RPC_test : Parent Thread released the lock %d\n\n", AcquiredLockIndexAcquireTest); 
		
	Exit(0);
}

/***********************************************************************************************/
/*								RELEASE LOCK RPC TEST CASE						   			   */	
/***********************************************************************************************/
void ChildThreadReleaseLock_test() {
	
	int lockIndex = 0;
	Print_Stmt("Inside child thread release\n");
	/*------------------------------------------------------------------------*/
	/*	CHILD THREAD TRYING TO RELEASE A LOCK ACQUIRED BY PARENT THREAD  */	
	/*------------------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t		CHILD THREAD TRYING TO RELEASE A LOCK ACQUIRED BY PARENT THREAD\n");
	lockIndex = Release(AcquiredLockIndexReleaseTest);	
	if(lockIndex == -1){
		Print_1Arg("ChildThreadReleaseLock_test : Child could not Release the Lock %d\n", AcquiredLockIndexReleaseTest);
	}else	
		Print_1Arg("ChildThreadReleaseLock_test : Child Released the Lock %d Acquired by parent\n", AcquiredLockIndexReleaseTest);
	
	Exit(0);
}

void ReleaseLock_RPC_test() {

	int createdlockIndex = 0;
	int lockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tRELEASE LOCK RPC TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE A LOCK WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : CREATE A LOCK WITH VALID PARAMETERS\n");
	createdlockIndex = CreateLock("ReleaseTestLock", 16);
	if(createdlockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Created\n");
	}else	
		Print_1Arg("ReleaseLock_RPC_test : Lock Created with Index %d\n", createdlockIndex);
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : ACQUIRE A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = Acquire(createdlockIndex);	
	if(lockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Acquired\n");
	}else
		Print_1Arg("ReleaseLock_RPC_test : Lock Acquired with Index %d\n", lockIndex);

	/*-----------------------------------------------------------------*/
	/*					RELEASE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		RELEASE A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = Release(lockIndex);	
	if(lockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Released\n");
	}else	
		Print_1Arg("ReleaseLock_RPC_test : Lock Released with Index %d\n", lockIndex);
		
	/*-----------------------------------------------------------------*/
	/*					RELEASE A LOCK WITH INVALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		RELEASE A LOCK WITH INVALID LOCK INDEX\n");
	lockIndex = Release(-10);	
	if(lockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Released\n");
	}else	
		Print_1Arg("ReleaseLock_RPC_test : Lock Released with Index %d\n", lockIndex);	
	
	/*-----------------------------------------------------------------*/
	/*					RELEASE A LOCK WHICH IS NOT CREATED		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		RELEASE A LOCK WHICH IS NOT CREATED\n");
	lockIndex = Release(3);	
	if(lockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Released\n");
	}else	
		Print_1Arg("ReleaseLock_RPC_test : Lock Released with Index %d\n", lockIndex);	
		
	/*-----------------------------------------------------------------*/
	/*	MAKE CHILD THREAD RELEASE A LOCK ACQUIRED BY CURRENT THREAD    */	
	/*-----------------------------------------------------------------*/
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		MAKE CHILD THREAD RELEASE A LOCK ACQUIRED BY CURRENT THREAD\n");
	lockIndex = Acquire(createdlockIndex);	
	if(lockIndex == -1){
		Print_Stmt("ReleaseLock_RPC_test : Lock was not Acquired\n");
	}else{	
		AcquiredLockIndexReleaseTest = lockIndex;
		Print_1Arg("ReleaseLock_RPC_test : Parent Acquired Lock with Index %d\n", lockIndex);
	}	
	Fork(ChildThreadReleaseLock_test);
	
	Exit(0);	
}

/***********************************************************************************************/
/*									DESTROY LOCK RPC TEST CASE								   */		
/***********************************************************************************************/
void Destroy_Lock_W1_Create() {

	int createdlockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 1\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE A LOCK WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : CREATE A LOCK WITH VALID PARAMETERS\n");
	createdlockIndex = CreateLock("DestroyTestLock", 16);
	if(createdlockIndex == -1){
		Print_Stmt("Destroy_W1_Create : Lock was not Created\n");
	}else	
		Print_1Arg("Destroy_W1_Create : Lock Created with Index %d\n", createdlockIndex);
		
	return;	
}
					
void Destroy_Lock_W2_Create() {

	int createdlockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 2\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE A LOCK WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : CREATE A LOCK WITH VALID PARAMETERS\n");
	createdlockIndex = CreateLock("DestroyTestLock", 16);
	if(createdlockIndex == -1){
		Print_Stmt("Destroy_W2_Create : Lock was not Created\n");
	}else	
		Print_1Arg("Destroy_W2_Create : Lock Created with Index %d\n", createdlockIndex);
		
	return;	
}

void Destroy_Lock_W1_Acquire() {
	
	int lockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 3\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tINITIAL SETUP : ACQUIRE A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = Acquire(0);	
	if(lockIndex == -1){
		Print_Stmt("DestroyLock_RPC_test : Lock was not Acquired\n");
	}else
		Print_1Arg("DestroyLock_RPC_test : Lock Acquired with Index %d\n", lockIndex);
	
	return;
}

void Destroy_Lock_W1_Destroy() {

	int lockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 4\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					DESTROY A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tTEST1 : DESTROY A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = DestroyLock(0);	
	if(lockIndex == -1){
		Print_Stmt("DestroyLock_RPC_test : Lock was not Destroyed\n");
	}else
		Print_1Arg("DestroyLock_RPC_test : Lock Destroyed with Index %d\n", lockIndex);
	
	return;	
}

void Destroy_Lock_W2_Destroy() {

	int lockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 5\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					DESTROY A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\tTEST1 : DESTROY A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = DestroyLock(0);	
	if(lockIndex == -1){
		Print_Stmt("DestroyLock_RPC_test : Lock was not Destroyed\n");
	}else
		Print_1Arg("DestroyLock_RPC_test : Lock Destroyed with Index %d\n", lockIndex);
	
	return;	
}

void Destroy_Lock_W1_Release() {

	int lockIndex = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY LOCK RPC TEST CASE - STEP 6\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					RELEASE A LOCK WITH VALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		RELEASE A LOCK WITH VALID LOCK INDEX\n");
	lockIndex = Release(0);	
	if(lockIndex == -1){
		Print_Stmt("DestroyLock_RPC_test : Lock was not Released\n");
	}else	
		Print_1Arg("DestroyLock_RPC_test : Lock Released with Index %d\n", lockIndex);
		
	return;	
}
					

/***********************************************************************************************/
/*								CREATE CONDITION VARIABLE RPC TEST CASE						   */	
/***********************************************************************************************/
void CreateCV_RPC_test() {

	int cvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tCREATE CV RPC TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV WITH VALID PARAMETERS			   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		CREATE CV WITH VALID PARAMETERS\n");
	cvIndex = CreateCV("newCV", 6);
	if(cvIndex == -1){
		Print_Stmt("CreateCV_RPC_test : CV was not Created\n");
	}else	
		Print_1Arg("CreateCV_RPC_test : CV Created with Index %d\n", cvIndex);
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV WITH INVALID LENGTH			       */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		CREATE CV WITH INVALID LENGTH\n");
	cvIndex = 0;
	cvIndex = CreateCV("newCV2", -1);
	if(cvIndex == -1){
		Print_Stmt("CreateCV_RPC_test : CV was not Created\n");
	}else	
		Print_1Arg("CreateCV_RPC_test : CV Created with Index %d\n", cvIndex);
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV WITH THE SAME NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		CREATE CV WITH THE SAME NAME\n");
	cvIndex = 0;
	cvIndex = CreateCV("newCV", 6);
	if(cvIndex == -1){
		Print_Stmt("CreateCV_RPC_test : CV was not Created\n");
	}else	
		Print_1Arg("CreateCV_RPC_test : CV Created with Index %d\n", cvIndex);
		
	/*-----------------------------------------------------------------*/
	/*					CREATE CV WITH NO NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		CREATE CV WITH NO NAME\n");
	cvIndex = 0;
	cvIndex = CreateCV("", 1);
	if(cvIndex == -1){
		Print_Stmt("CreateCV_RPC_test : CV was not Created\n");
	}else	
		Print_1Arg("CreateCV_RPC_test : CV Created with Index %d\n", cvIndex);	
	
	Exit(0);
}

/***********************************************************************************************/
/*								WAIT CV RPC TEST CASE						   				   */	
/***********************************************************************************************/
void WaitCV_RPC_test() {

	int cvIndex = -1;
	int lockIndex = -1;
	int createdlockIndex = -1;
	int unCreatedlockIndex = 12;
	int createdcvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tWAIT CV RPC TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV FOR CONDITION WAITING			   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR CONDITION WAITING\n");
	cvIndex = CreateCV("waitCV", 7);
	if(cvIndex == -1){
		Print_Stmt("WaitCV_RPC_test : CV was not Created for condition waiting\n");
	}else{
		createdcvIndex = cvIndex;
		Print_1Arg("WaitCV_RPC_test : CV Created with Index %d for condition waiting\n", cvIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK FOR CONDITION WAITING			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE LOCK FOR CONDITION WAITING\n");
	lockIndex = CreateLock("waitLock", 9);
	if(lockIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Lock was not Created for condition waiting\n");
	}else{	
		createdlockIndex = lockIndex;
		Print_1Arg("WaitCV_RPC_test : Lock Created with Index %d for condition waiting\n", lockIndex);
	}	

	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR CONDITION WAITING		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK FOR CONDITION WAITING\n");
	lockIndex = Acquire(createdlockIndex);	
	if(lockIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Lock was not Acquired for condition waiting\n");
	}else	
		Print_1Arg("WaitCV_RPC_test : Lock Acquired with Index %d for condition waiting\n", lockIndex);	
	
	/*-----------------------------------------------------------------*/
	/*				CONDITION WAITING WITH INVALID LOCK INDEX		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		CONDITION WAITING WITH INVALID LOCK INDEX\n");
	cvIndex = Wait(createdcvIndex, -10);
	if(cvIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Wait unsuccessful\n");
	}else	
		Print_1Arg("WaitCV_RPC_test : Thread is back from wait on CV %d\n", cvIndex);
	
	/*-----------------------------------------------------------------*/
	/*				CONDITION WAITING WITH INVALID CV INDEX		       */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		CONDITION WAITING WITH INVALID CV INDEX\n");
	cvIndex = Wait(-10, createdlockIndex);
	if(cvIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Wait unsuccessful\n");
	}else	
		Print_1Arg("WaitCV_RPC_test : Thread is back from wait on CV %d\n", cvIndex);
		
	/*-----------------------------------------------------------------*/
	/*			CONDITION WAITING WITH A LOCK NOT OWNED BY THIS THREAD */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		CONDITION WAITING WITH A LOCK NOT OWNED BY THIS THREAD\n");
	cvIndex = Wait(createdcvIndex, unCreatedlockIndex);
	if(cvIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Wait unsuccessful\n");
	}else	
		Print_1Arg("WaitCV_RPC_test : Thread is back from wait on CV %d\n", cvIndex);	
		
	/*-----------------------------------------------------------------*/
	/*				CONDITION WAITING WITH VALID LOCK AND CV		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		CONDITION WAITING WITH VALID LOCK AND CV\n");
	Print_Stmt("NOTE TO GRADER: Since the current machine has been put into wait\n");
	Print_Stmt("\t\tyou can see the server has not replied to the client.\n\n");
	Print_Stmt("\t\tTo make the current machine come out of wait, you need to open another\n");
	Print_Stmt("\t\tWindow with a different instance of Nachos running on it. Once you see \n");
	Print_Stmt("\t\tthe TEST CASES MENU, please select the Signal RPC test case in it.\n\n");
	
	cvIndex = Wait(createdcvIndex, createdlockIndex);
	if(cvIndex == -1){
		Print_Stmt("WaitCV_RPC_test : Wait unsuccessful\n");
	}else	
		Print_1Arg("WaitCV_RPC_test : Thread is back from wait on CV %d\n", cvIndex);
		
		/*-----------------------------------------------------------------*/
		/*					RELEASE A LOCK AFTER COMING OUT OF WAIT 	   */	
		/*-----------------------------------------------------------------*/
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\n\tRELEASE A LOCK AFTER COMING OUT OF WAIT\n");
		lockIndex = Release(createdlockIndex);	
		if(lockIndex == -1){
			Print_Stmt("WaitCV_RPC_test : Could not Release the Lock after coming out of wait\n");
		}else	
			Print_1Arg("WaitCV_RPC_test : Release the Lock %d after coming out of wait\n", lockIndex);
	
	Exit(0);		
}

/***********************************************************************************************/
/*								SIGNAL CV RPC TEST CASE						   				   */	
/***********************************************************************************************/
void SignalCV_RPC_test() {
	
	int waitingLockIndex = 0;	/*  Assumed to be known to this machine */
	int waitingCVIndex = 0;		/* Assumed to be known to this machine */
	int lockIndex = 0;
	int cvIndex = 0;
	int waitingLockIndexMismatch = 0;
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK FOR SIGNALLING A WAITER				   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE LOCK FOR SIGNALLING A WAITER\n");
	waitingLockIndex = CreateLock("waitLock", 9);
	if(waitingLockIndex == -1){
		Print_Stmt("SignalCV_RPC_test : Lock was not Created for signalling a waiter\n");
	}else{	
		Print_1Arg("SignalCV_RPC_test : Lock Created with Index %d for signalling a waiter\n", waitingLockIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV FOR SIGNALLING A WAITER			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR SIGNALLING A WAITER\n");
	waitingCVIndex = CreateCV("waitCV", 7);
	if(waitingCVIndex == -1){
		Print_Stmt("SignalCV_RPC_test : CV was not Created for signalling a waiter\n");
	}else{
		Print_1Arg("SignalCV_RPC_test : CV Created with Index %d for signalling a waiter\n", waitingCVIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR SIGNALLING A WAITER		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK FOR SIGNALLING A WAITER\n");
	lockIndex = Acquire(waitingLockIndex);	
	if(lockIndex == -1){
		Print_Stmt("SignalCV_RPC_test : Lock was not Acquired for signalling a waiter\n");
	}else	
		Print_1Arg("SignalCV_RPC_test : Lock %d Acquired for signalling a waiter\n", lockIndex);	

	/*-----------------------------------------------------------------*/
	/*					SIGNALLING A WAITER WITH AN INVALID CV INDEX   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		SIGNALLING A WAITER WITH AN INVALID CV INDEX\n");
	cvIndex = Signal(10, waitingLockIndex);	
	if(cvIndex == -1){
		Print_1Arg("SignalCV_RPC_test : Could not Signal a waiter waiting on CV 10 using the Lock %d\n", waitingLockIndex);
	}else{	
		Print_1Arg("SignalCV_RPC_test : Signalled a waiter(if any) waiting on CV 10 using the Lock %d\n", waitingLockIndex);
	}	
	/*-----------------------------------------------------------------*/
	/*					SIGNALLING A WAITER WITH AN INVALID LOCK INDEX */	
	/*-----------------------------------------------------------------*/
			/*-----------------------------------------------------------------*/
			/*	CREATE LOCK FOR SIGNALLING A WAITER WITH A DIFFERENT LOCK	   */	
			/*-----------------------------------------------------------------*/
			Print_Stmt("-----------------------------------------------------------------\n");
			Print_Stmt("\n\tINITIAL TEST 2 SETUP : CREATE LOCK FOR SIGNALLING A WAITER WITH A DIFFERENT LOCK\n");
			waitingLockIndexMismatch = CreateLock("waitLock2", 10);
			if(waitingLockIndexMismatch == -1){
				Print_1Arg("SignalCV_RPC_test : Lock was not Created for signalling a waiter with a different Lock %d\n", waitingLockIndexMismatch);
			}else{	
				Print_1Arg("SignalCV_RPC_test : Lock Created with Index %d for signalling a waiter with a different Lock %d\n", waitingLockIndexMismatch);
			}	
			/*-----------------------------------------------------------------*/
			/*	ACQUIRE A LOCK FOR SIGNALLING A WAITER WITH A DIFFERENT LOCK   */	
			/*-----------------------------------------------------------------*/
			Print_Stmt("-----------------------------------------------------------------\n");
			Print_Stmt("\n\tINITIAL TEST 2 SETUP : ACQUIRE A LOCK FOR SIGNALLING A WAITER WITH A DIFFERENT LOCK\n");
			waitingLockIndexMismatch = Acquire(waitingLockIndexMismatch);	
			if(waitingLockIndexMismatch == -1){
				Print_1Arg("SignalCV_RPC_test : Lock was not Acquired for signalling a waiter with a different Lock %d\n", waitingLockIndexMismatch);
			}else	
				Print_1Arg("SignalCV_RPC_test : Lock %d Acquired for signalling a waiter with a different Lock to create Lock Mismatch\n", waitingLockIndexMismatch);
			Print_Stmt("-----------------------------------------------------------------\n");
			Print_Stmt("\n\tTEST2		SIGNALLING A WAITER WITH AN INVALID LOCK INDEX TO MISMATCH THE CV LOCK\n");
			cvIndex = Signal(waitingCVIndex, waitingLockIndexMismatch);	
			if(cvIndex == -1){
				Print_2Arg("SignalCV_RPC_test : Could not Signal a waiter waiting on CV %d using the Lock %d\n", waitingCVIndex, waitingLockIndexMismatch);
			}else{	
				Print_2Arg("SignalCV_RPC_test : Signalled a waiter(if any) waiting on CV %d using the Lock %d\n", waitingCVIndex, waitingLockIndexMismatch);	
			}
			/*-----------------------------------------------------------------*/
			/*	RELEASE A LOCK AFTER SIGNALLING A WAITER WITH A DIFFERENT LOCK */	
			/*-----------------------------------------------------------------*/
			Print_Stmt("-----------------------------------------------------------------\n");
			Print_Stmt("\n\tRELEASE A LOCK AFTER SIGNALLING A WAITER WITH A DIFFERENT LOCK\n");
			lockIndex = Release(waitingLockIndexMismatch);	
			if(lockIndex == -1){
				Print_1Arg("SignalCV_RPC_test : Could not Release the Lock after signalling a waiter with a different Lock %d\n", waitingLockIndexMismatch);
			}else	
				Print_1Arg("SignalCV_RPC_test : Release the Lock %d after signalling a waiter\n", waitingLockIndexMismatch);
			
	/*-----------------------------------------------------------------*/
	/*	SIGNALLING A WAITER WAITING ON CV WITH VALID LOCK		   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_1Arg("\n\tTEST3		SIGNALLING A WAITER WAITING ON CV WITH VALID LOCK\n", waitingCVIndex);
	cvIndex = Signal(waitingCVIndex, waitingLockIndex);	
	if(cvIndex == -1){
		Print_2Arg("SignalCV_RPC_test : Could not Signal a waiter waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}else{	
		Print_2Arg("SignalCV_RPC_test : Signalled a waiter(if any) waiting on CV %d using the Lock %\n", cvIndex, waitingLockIndex);
	}	
		/*-----------------------------------------------------------------*/
		/*					RELEASE A LOCK AFTER SIGNALLING A WAITER		   */	
		/*-----------------------------------------------------------------*/
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\n\tRELEASE A LOCK AFTER SIGNALLING A WAITER\n");
		lockIndex = Release(waitingLockIndex);	
		if(lockIndex == -1){
			Print_Stmt("SignalCV_RPC_test : Could not Release the Lock after signalling a waiter\n");
		}else	
			Print_1Arg("SignalCV_RPC_test : Release the Lock %d after signalling a waiter\n", lockIndex);
		
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\nNOTE TO GRADER : (If any waiters were there in the CV wait queue) Now you can see\n");
		Print_Stmt("\t\tthe machine which was waiting on the CV on the other window is back\n");
		Print_Stmt("\t\tfrom WAIT.");	
		
	Exit(0);	
}


/***********************************************************************************************/
/*								BROADCAST CV RPC TEST CASE 									   */	
/*			TO MAKE MACHINES GO ON WAIT BEFORE BROADCASTING A SIGNAL : STEP1-STEP3	   		   */	
/***********************************************************************************************/
void BroadcastCV_Wait_RPC_test() {

	int cvIndex = -1;
	int lockIndex = -1;
	int createdlockIndex = -1;
	int unCreatedlockIndex = 4;
	int createdcvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------------------------------\n");
	Print_Stmt("\t\tBROADCAST CV RPC TEST CASE	- TO MAKE MACHINES GO ON WAIT BEFORE BROADCASTING A SIGNAL\n");
	Print_Stmt("-------------------------------------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV FOR CONDITION WAITING			   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR CONDITION WAITING \n");
	cvIndex = CreateCV("waitCV", 7);
	if(cvIndex == -1){
		Print_Stmt("BroadcastCV_Wait_RPC_test : CV was not Created for condition waiting\n");
	}else{
		createdcvIndex = cvIndex;
		Print_1Arg("BroadcastCV_Wait_RPC_test : CV Created with Index %d for condition waiting\n", cvIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK FOR CONDITION WAITING			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE LOCK FOR CONDITION WAITING\n");
	lockIndex = CreateLock("waitLock", 9);
	if(lockIndex == -1){
		Print_Stmt("BroadcastCV_Wait_RPC_test : Lock was not Created for condition waiting\n");
	}else{	
		createdlockIndex = lockIndex;
		Print_1Arg("BroadcastCV_Wait_RPC_test : Lock Created with Index %d for condition waiting\n", lockIndex);
	}	

	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR CONDITION WAITING		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK FOR CONDITION WAITING\n");
	lockIndex = Acquire(createdlockIndex);	
	if(lockIndex == -1){
		Print_Stmt("BroadcastCV_Wait_RPC_test : Lock was not Acquired for condition waiting\n");
	}else	
		Print_1Arg("BroadcastCV_Wait_RPC_test : Lock Acquired with Index %d for condition waiting\n", lockIndex);	
	
	/*-----------------------------------------------------------------*/
	/*				CONDITION WAITING WITH VALID LOCK AND CV		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST		CONDITION WAITING WITH VALID LOCK AND CV\n");
	Print_Stmt("NOTE TO GRADER: Since the current machine has been put into wait\n");
	Print_Stmt("\t\tyou can see the server has not replied to the client.\n\n");
	Print_Stmt("\t\tPlease complete the 3 STEPS to make 3 machines go on wait on same CV\n");
	Print_Stmt("\t\tIf you are in STEP 3 then open one more window with a different\n");
	Print_Stmt("\t\tinstance of Nachos running on it. Once you see the TEST CASES MENU\n");
	Print_Stmt("\t\tPlease select the STEP 4 to broadcast the signal to all the waiters\n\n");
	
	cvIndex = Wait(createdcvIndex, createdlockIndex);
	if(cvIndex == -1){
		Print_Stmt("BroadcastCV_Wait_RPC_test : Wait unsuccessful\n");
	}else{	
		Print_1Arg("BroadcastCV_Wait_RPC_test : Thread is back from wait on CV %d\n", cvIndex);
	}	
		/*-----------------------------------------------------------------*/
		/*					RELEASE A LOCK AFTER COMING OUT OF WAIT 	   */	
		/*-----------------------------------------------------------------*/
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\n\tRELEASE A LOCK AFTER COMING OUT OF WAIT\n");
		lockIndex = Release(createdlockIndex);	
		if(lockIndex == -1){
			Print_Stmt("BroadcastCV_Wait_RPC_test : Could not Release the Lock after coming out of wait\n");
		}else	
			Print_1Arg("BroadcastCV_Wait_RPC_test : Release the Lock %d after coming out of wait\n", lockIndex);
	
	Exit(0);			
}


/***********************************************************************************************/
/*								BROADCAST CV RPC TEST CASE - STEP 4						   	   */	
/***********************************************************************************************/
void BroadcastCV_RPC_test() {

	int waitingLockIndex = 0;	
	int waitingCVIndex = 0;		
	int lockIndex = 0;
	int cvIndex = 0;
	
	/*-----------------------------------------------------------------*/
	/*					CREATE LOCK FOR BROADCASTING THE WAITERS	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP :CREATE LOCK FOR BROADCASTING THE WAITERS\n");
	waitingLockIndex = CreateLock("waitLock", 9);
	if(waitingLockIndex == -1){
		Print_Stmt("BroadcastCV_RPC_test : Lock was not Created for broadcasting the waiters\n");
	}else{	
		Print_1Arg("BroadcastCV_RPC_test : Lock Created with Index %d for broadcasting the waiters\n", waitingLockIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					CREATE CV FOR BROADCASTING THE WAITERS			   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR BROADCASTING THE WAITERS\n");
	waitingCVIndex = CreateCV("waitCV", 7);
	if(waitingCVIndex == -1){
		Print_Stmt("BroadcastCV_RPC_test : CV was not Created for broadcasting the waiters\n");
	}else{
		Print_1Arg("BroadcastCV_RPC_test : CV Created with Index %d for broadcasting the waiters\n", waitingCVIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR BROADCASTING THE WAITERS		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK BROADCASTING THE WAITERS\n");
	lockIndex = Acquire(waitingLockIndex);	
	if(lockIndex == -1){
		Print_Stmt("BroadcastCV_RPC_test : Lock was not Acquired for broadcasting the waiters\n");
	}else	
		Print_1Arg("BroadcastCV_RPC_test : Lock %d Acquired for broadcasting the waiters\n", lockIndex);	

	/*-----------------------------------------------------------------*/
	/*					BROADCASTING THE WAITERS WITH INVALID LOCK	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_1Arg("\n\tTEST1 :		BROADCASTING THE WAITERS WITH INVALID LOCK\n", waitingCVIndex);
	cvIndex = Broadcast(waitingCVIndex, -10);	
	if(cvIndex == -1){
		Print_2Arg("BroadcastCV_RPC_test : Could not broadcast the waiters waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}else{	
		Print_2Arg("BroadcastCV_RPC_test : Broadcasted the waiters(if any) waiting on CV %d using the Lock %\n", cvIndex, waitingLockIndex);
	}	
	/*-----------------------------------------------------------------*/
	/*					BROADCASTING THE WAITERS WITH INVALID CV	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_1Arg("\n\tTEST2 :		BROADCASTING THE WAITERS WITH INVALID CV\n", waitingCVIndex);
	cvIndex = Broadcast(-10, waitingLockIndex);	
	if(cvIndex == -1){
		Print_2Arg("BroadcastCV_RPC_test : Could not broadcast the waiters waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}else{	
		Print_2Arg("BroadcastCV_RPC_test : Broadcasted the waiters(if any) waiting on CV %d using the Lock %\n", cvIndex, waitingLockIndex);	
	}	
	/*-----------------------------------------------------------------*/
	/*	BROADCASTING THE WAITERS WAITING WITH VALID CV AND VALID LOCK  */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3 :		BROADCASTING THE WAITERS WAITING WITH VALID CV AND VALID LOCK\n");
	cvIndex = Broadcast(waitingCVIndex, waitingLockIndex);	
	if(cvIndex == -1){
		Print_2Arg("BroadcastCV_RPC_test : Could not broadcast the waiters waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}else{	
		Print_2Arg("BroadcastCV_RPC_test : Broadcasted the waiters(if any) waiting on CV %d using the Lock %\n", cvIndex, waitingLockIndex);
		
		/*-----------------------------------------------------------------*/
		/*					RELEASE A LOCK AFTER BROADCASTING THE WAITERS		   */	
		/*-----------------------------------------------------------------*/
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\n\tRELEASE A LOCK AFTER BROADCASTING THE WAITERS\n");
		lockIndex = Release(waitingLockIndex);	
		if(lockIndex == -1){
			Print_Stmt("BroadcastCV_RPC_test : Could not Release the Lock after broadcasting the waiters\n");
		}else	
			Print_1Arg("BroadcastCV_RPC_test : Release the Lock %d after broadcasting the waiters\n", lockIndex);
		
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\nNOTE TO GRADER : If any of the first 3 STEPS were executed before this STEP 4\n");
		Print_Stmt("\t\tThen now you can see those machines which were waiting on the CV are back from WAIT\n");
	}	
		
	Exit(0);
}


/***********************************************************************************************/
/*								DESTROY CV RPC TEST CASE - STEP 1						   	   */	
/***********************************************************************************************/
void DestroyCV_Wait_RPC_test() {
	
	int waitingLockIndex = 0;	/*  Assumed to be known to this machine */
	int waitingCVIndex = 0;		/* Assumed to be known to this machine */
	int lockIndex = 0;
	int cvIndex = 0;
	
	/*-----------------------------------------------------------------*/
	/*	CREATE LOCK FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE 	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE LOCK FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE\n");
	waitingLockIndex = CreateLock("DestroyCVLock", 14);
	if(waitingLockIndex == -1){
		Print_Stmt("DestroyCV_Wait_RPC_test : Lock was not Created for destroying a CV after wait-signal is done\n");
	}else{	
		Print_1Arg("DestroyCV_Wait_RPC_test : Lock %d Created for destroying a CV after wait-signal is done\n", waitingLockIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/* CREATE CV FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE\n");
	waitingCVIndex = CreateCV("waitCV", 7);
	if(waitingCVIndex == -1){
		Print_Stmt("DestroyCV_Wait_RPC_test : CV was not Created for destroying a CV after wait-signal is done\n");
	}else{
		Print_1Arg("DestroyCV_Wait_RPC_test : CV %d Created for destroying a CV after wait-signal is done\n", waitingCVIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR GOING IN WAIT ON CV 		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK FOR GOING IN WAIT ON CV\n");
	lockIndex = Acquire(waitingLockIndex);	
	if(lockIndex == -1){
		Print_Stmt("DestroyCV_Wait_RPC_test : Lock was not Acquired for going on wait\n");
	}else	
		Print_1Arg("DestroyCV_Wait_RPC_test : Lock %d Acquired for going on wait\n", lockIndex);	
	
	/*-----------------------------------------------------------------*/
	/*				CONDITION WAITING WITH VALID LOCK AND CV		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		CONDITION WAITING WITH VALID LOCK AND CV\n");
	Print_Stmt("NOTE TO GRADER: Since the current machine has been put into wait\n");
	Print_Stmt("\t\tyou can see the server has not replied to the client.\n\n");
	Print_Stmt("\t\tTo make the current machine come out of wait, you need to open another\n");
	Print_Stmt("\t\tWindow with a different instance of Nachos running on it. Once you see \n");
	Print_Stmt("\t\tthe TEST CASES MENU, please select the \"STEP 2\" under \"Destroy CV test case\"\n\n");
	
	cvIndex = Wait(waitingCVIndex, waitingLockIndex);
	if(cvIndex == -1){
		Print_Stmt("DestroyCV_Wait_RPC_test : Wait unsuccessful\n");
	}else{	
		Print_1Arg("DestroyCV_Wait_RPC_test : Thread is back from wait on CV %d\n", cvIndex);
	}	
		/*-----------------------------------------------------------------*/
		/*					RELEASE A LOCK AFTER COMING OUT OF WAIT 	   */	
		/*-----------------------------------------------------------------*/
		Print_Stmt("-----------------------------------------------------------------\n");
		Print_Stmt("\n\tRELEASE A LOCK AFTER COMING OUT OF WAIT\n");
		lockIndex = Release(waitingLockIndex);	
		if(lockIndex == -1){
			Print_Stmt("DestroyCV_Wait_RPC_test : Could not Release the Lock after coming out of wait\n");
		}else	
			Print_1Arg("DestroyCV_Wait_RPC_test : Release the Lock %d after coming out of wait\n", lockIndex);
			
	/*-----------------------------------------------------------------*/
	/*					DESTROY A CV WHICH AFTER COMING OUT OF WAIT 		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1 : DESTROY A CV AFTER COMING OUT OF WAIT \n");
	lockIndex = DestroyCV(waitingCVIndex);	
	if(lockIndex == -1){
		Print_Stmt("DestroyCV_Wait_RPC_test : CV was not destroyed\n");
	}else	
		Print_1Arg("DestroyCV_Wait_RPC_test : CV was Destroyed\n", lockIndex);			
			
	Exit(0);		
}


/***********************************************************************************************/
/*								DESTROY CV RPC TEST CASE - STEP 2						   	   */	
/***********************************************************************************************/
void DestroyCV_Signal_RPC_test() {
	
	int waitingLockIndex = 0;	/*  Assumed to be known to this machine */
	int waitingCVIndex = 0;		/* Assumed to be known to this machine */
	int lockIndex = 0;
	int cvIndex = 0;
	
	/*-----------------------------------------------------------------*/
	/*	CREATE LOCK FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE 	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE LOCK FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE\n");
	waitingLockIndex = CreateLock("DestroyCVLock", 14);
	if(waitingLockIndex == -1){
		Print_Stmt("DestroyCV_Signal_RPC_test : Lock was not Created for destroying a CV after wait-signal is done\n");
	}else{	
		Print_1Arg("DestroyCV_Signal_RPC_test : Lock %d Created for destroying a CV after wait-signal is done\n", waitingLockIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/* CREATE CV FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE	   	   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : CREATE CV FOR DESTROYING A CV AFTER WAIT-SIGNAL IS DONE\n");
	waitingCVIndex = CreateCV("waitCV", 7);
	if(waitingCVIndex == -1){
		Print_Stmt("DestroyCV_Signal_RPC_test : CV was not Created for destroying a CV after wait-signal is done\n");
	}else{
		Print_1Arg("DestroyCV_Signal_RPC_test : CV %d Created for destroying a CV after wait-signal is done\n", waitingCVIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/*					ACQUIRE A LOCK FOR GOING IN WAIT ON CV 		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP : ACQUIRE A LOCK FOR GOING ON WAIT IN CV\n");
	lockIndex = Acquire(waitingLockIndex);	
	if(lockIndex == -1){
		Print_Stmt("DestroyCV_Signal_RPC_test : Lock was not Acquired for signalling a waiter\n");
	}else	
		Print_1Arg("DestroyCV_Signal_RPC_test : Lock %d Acquired for signalling a waiter\n", lockIndex);	
	
	/*-----------------------------------------------------------------*/
	/*					DESTROY A CV WHICH HAS A WAITER IN IT		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1 : DESTROY A CV WHICH HAS A WAITER IN IT\n");
	lockIndex = DestroyCV(waitingCVIndex);	
	if(lockIndex == -1){
		Print_Stmt("DestroyCV_Signal_RPC_test : CV was not destroyed\n");
	}else	
		Print_1Arg("DestroyCV_Signal_RPC_test : CV was Destroyed\n", lockIndex);	
		
	/*-----------------------------------------------------------------*/
	/*					SIGNALLING A WAITER WAITING ON CV		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_1Arg("\n\tTEST3		SIGNALLING A WAITER WAITING ON CV %d\n", waitingCVIndex);
	cvIndex = Signal(waitingCVIndex, waitingLockIndex);	
	if(cvIndex == -1){
		Print_2Arg("DestroyCV_Signal_RPC_test : Could not Signal a waiter waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}else{	
		Print_2Arg("DestroyCV_Signal_RPC_test : Signalled a waiter(if any) waiting on CV %d using the Lock %\n", waitingCVIndex, waitingLockIndex);
	}
	
	/*-----------------------------------------------------------------*/
	/*					RELEASE A LOCK AFTER SIGNALLING A WAITER		   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tRELEASE A LOCK AFTER SIGNALLING A WAITER\n");
	lockIndex = Release(waitingLockIndex);	
	if(lockIndex == -1){
		Print_Stmt("DestroyCV_Signal_RPC_test : Could not Release the Lock after signalling a waiter\n");
	}else	
		Print_1Arg("DestroyCV_Signal_RPC_test : Release the Lock %d after signalling a waiter\n", lockIndex);
	
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\nNOTE TO GRADER : (If any waiters were there in the CV wait queue) Now you can see\n");
	Print_Stmt("\t\tthe machine which was waiting on the CV on the other window is back\n");
	Print_Stmt("\t\tfrom WAIT. Now the Wait queue is empty so the CV is deallocated successfully\n");
		
	
	Exit(0);
}


/***********************************************************************************************/
/*								CREATE MONITOR VARIABLE	TEST CASE						   	   */	
/***********************************************************************************************/
void CreateMV_RPC_test() {

	int mvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tCREATE MONITOR VARIABLE	TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		CREATE MV WITH VALID PARAMETERS\n");
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("CreateMV_RPC_test : MV was not Created\n");
	}else	
		Print_1Arg("CreateMV_RPC_test : MV Created with Index %d\n", mvIndex);
	
	/*-----------------------------------------------------------------*/
	/* 					CREATE MV WITH INVALID LENGTH				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		CREATE MV WITH INVALID LENGTH\n");
	
	mvIndex = CreateMV("newMV2", -1);
	if(mvIndex == -1){
		Print_Stmt("CreateMV_RPC_test : MV was not Created\n");
	}else	
		Print_1Arg("CreateMV_RPC_test : MV Created with Index %d\n", mvIndex);
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH THE SAME NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		CREATE MV WITH THE SAME NAME\n");
	
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("CreateMV_RPC_test : MV was not Created\n");
	}else	
		Print_1Arg("CreateMV_RPC_test : MV Created with Index %d\n", mvIndex);
		
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH NO NAME				   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST4		CREATE MV WITH NO NAME\n");

	mvIndex = CreateMV("",1);
	if(mvIndex == -1){
		Print_Stmt("CreateMV_RPC_test : MV was not Created\n");
	}else	
		Print_1Arg("CreateMV_RPC_test : MV Created with Index %d\n", mvIndex);	
	
	Exit(0);	

}


/***********************************************************************************************/
/*								GET MONITOR VARIABLE	TEST CASE						   	   */	
/***********************************************************************************************/
void GetMV_RPC_test() {

	int mvIndex = -1;
	int createdMV = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tGET MONITOR VARIABLE	TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP :		CREATE MV WITH VALID PARAMETERS\n");
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("GetMV_RPC_test : MV was not Created\n");
	}else{
		createdMV = mvIndex;	
		Print_1Arg("GetMV_RPC_test : MV Created with Index %d\n", mvIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/* 					GET MV VALUE WITH INVALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		GET MV VALUE WITH INVALID MV INDEX\n");
	
	mvIndex = GetMV(-1);
	if(mvIndex == -1){
		Print_Stmt("GetMV_RPC_test : MV value was not fetched from server\n");
	}else	
		Print_1Arg("GetMV_RPC_test : MV value fetched from server is %d\n", mvIndex);
	
	/*-----------------------------------------------------------------*/
	/* 					GET MV VALUE WITH VALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		GET MV VALUE WITH VALID MV INDEX\n");
	
	mvIndex = GetMV(createdMV);
	if(mvIndex == -1){
		Print_Stmt("GetMV_RPC_test : MV value was not fetched from server\n");
	}else	
		Print_1Arg("GetMV_RPC_test : MV value was fetched from server is %d\n", mvIndex);
	
	Exit(0);	

}


/***********************************************************************************************/
/*								SET MONITOR VARIABLE	TEST CASE						   	   */	
/***********************************************************************************************/
void SetMV_RPC_test() {

	int mvIndex = -1;
	int createdMV = 0;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tSET MONITOR VARIABLE	TEST CASE\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP :		CREATE MV WITH VALID PARAMETERS\n");
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("SetMV_RPC_test : MV was not Created\n");
	}else{
		createdMV = mvIndex;	
		Print_1Arg("SetMV_RPC_test : MV Created with Index %d\n", mvIndex);
	}	
	
	/*-----------------------------------------------------------------*/
	/* 					SET MV VALUE WITH INVALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		SET MV VALUE WITH INVALID MV INDEX\n");
	
	mvIndex = SetMV(-1, 10);
	if(mvIndex == -1){
		Print_Stmt("SetMV_RPC_test : MV value was not set in server\n");
	}else	
		Print_Stmt("SetMV_RPC_test : MV value was set to the value 10 in server\n");
	
	/*-----------------------------------------------------------------*/
	/* 					SET MV VALUE WITH VALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST2		SET MV VALUE WITH VALID MV INDEX\n");
	
	mvIndex = SetMV(createdMV, 10);
	if(mvIndex == -1){
		Print_Stmt("SetMV_RPC_test : MV value was not set to 10, in server\n");
	}else	
		Print_Stmt("SetMV_RPC_test : MV value was set to 10 in server\n");
		
	/*-----------------------------------------------------------------*/
	/* 					GET MV VALUE WITH VALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST3		GET MV VALUE WITH VALID MV INDEX\n");
	
	mvIndex = GetMV(createdMV);
	if(mvIndex == -1){
		Print_Stmt("SetMV_RPC_test : MV value was not fetched from server\n");
	}else	
		Print_1Arg("SetMV_RPC_test : MV value was fetched from server is %d\n", mvIndex);	
	
	Exit(0);	
}


/***********************************************************************************************/
/*								DESTROY MONITOR VARIABLE	TEST CASE						   	   */	
/***********************************************************************************************/
						
void Destroy_MV_W1_Create() {

	int mvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY MONITOR VARIABLE	TEST CASE - STEP 1\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP :		CREATE MV WITH VALID PARAMETERS\n");
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("Destroy_MV_W1_Create : MV was not Created\n");
	}else{
		Print_1Arg("Destroy_MV_W1_Create : MV Created with Index %d\n", mvIndex);
	}	
	
	return;
}

void Destroy_MV_W2_Create() {

	int mvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY MONITOR VARIABLE	TEST CASE - STEP 2\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/*					CREATE MV WITH VALID PARAMETERS			   */	
	/*-----------------------------------------------------------------*/
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\n\tINITIAL SETUP :		CREATE MV WITH VALID PARAMETERS\n");
	mvIndex = CreateMV("newMV", 6);
	if(mvIndex == -1){
		Print_Stmt("Destroy_MV_W2_Create : MV was not Created\n");
	}else{
		Print_1Arg("Destroy_MV_W2_Create : MV Created with Index %d\n", mvIndex);
	}	
	
	return;

} 

void Destroy_MV_W1_Destroy() {
	
	int mvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY MONITOR VARIABLE	TEST CASE - STEP 3\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/* 					DESTROY MV VALUE WITH VALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		DESTROY MV VALUE WITH VALID MV INDEX\n");
	
	mvIndex = DestroyMV(0);
	if(mvIndex == -1){
		Print_Stmt("Destroy_MV_W1_Destroy : MV was not destroyed\n");
	}else	
		Print_1Arg("Destroy_MV_W1_Destroy : MV %d was destroyedn", mvIndex);
		
	return;	
}

void Destroy_MV_W2_Destroy() {
	
	int mvIndex = -1;
	
	Print_Stmt("\n-----------------------------------------------------------------\n");
	Print_Stmt("\t\tDESTROY MONITOR VARIABLE	TEST CASE - STEP 4\n");
	Print_Stmt("-----------------------------------------------------------------\n");
	
	/*-----------------------------------------------------------------*/
	/* 					DESTROY MV VALUE WITH VALID MV INDEX					   */ 	
	/*-----------------------------------------------------------------*/
	Print_Stmt("-----------------------------------------------------------------\n");
	Print_Stmt("\n\tTEST1		DESTROY MV VALUE WITH VALID MV INDEX\n");
	
	mvIndex = DestroyMV(0);
	if(mvIndex == -1){
		Print_Stmt("Destroy_MV_W2_Destroy : MV was not destroyed\n");
	}else	
		Print_1Arg("Destroy_MV_W2_Destroy : MV %d was destroyed\n", mvIndex);
		
	return;	
}

int main() {
	
	int userInput = 0;
	char buf[3];
	
    /* Output statements for test case menu */
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n");
	Print_Stmt("\n\t\t			     PROJECT 3		  GROUP - 58\n\n");
	Print_Stmt("\t\t\t		     Virtual Memory and Basic Networking\n");
	Print_Stmt("\t\t\t		      >>>>>>>> TEST CASES PART 3 <<<<<<<<\n");
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n\n");
	
	Print_Stmt(" 1 > Create Lock RPC Test\n");
	Print_Stmt(" 2 > Acquire Lock RPC Test\n");
	Print_Stmt(" 3 > Release Lock RPC Test\n");
	Print_Stmt(" 4 > Destroy Lock RPC Test\n");
	Print_Stmt(" 5 > Create CV RPC Test\n");
	Print_Stmt(" 6 > Wait-Signal RPC Test\n");
	Print_Stmt(" 7 > Broadcast RPC Test\n");
	Print_Stmt(" 8 > Destroy CV RPC Test\n");
	Print_Stmt(" 9 > Create Monitor Variable RPC Test\n");
	Print_Stmt(" 10 > Get Monitor Variable RPC Test\n");
	Print_Stmt(" 11 > Set Monitor Variable RPC Test\n");
	Print_Stmt(" 12 > Destroy Monitor Variable RPC Test\n");
	Print_Stmt("\n\nEnter the index of the testcase you want to run here ->");
    
	userInput = Scan("%d"); 
	
 	switch(userInput){
		
		case 1:		Fork(CreateLock_RPC_test);
				break;
		case 2:		Fork(AcquireLock_RPC_test);
				break;
		case 3:		Fork(ReleaseLock_RPC_test);
				break;
	 	case 4:	Print_Stmt("\nNOTE TO THE GRADER : \nPlease restart the server and client for executing this test case\n");
				Print_Stmt("Because to make this test case work the Lock Index \n");
				Print_Stmt("of the Lock to be destroyed is fixed to zero '0'\n");
				Print_Stmt("\nIf above note is followed then select the following steps one by one\n");
				
				Print_Stmt("\tSTEP 1 > Make machine 1 Create a Lock ( Execute this step in window 1 )\n");
				Print_Stmt("\tSTEP 2 > Make machine 2 Create the same Lock ( Execute this step in window 2 )\n");
				Print_Stmt("\tSTEP 3 > Make machine 1 Acquire the Lock Created in STEP 1( Execute this step in window 1 )\n");
				Print_Stmt("\tSTEP 4 > Make machine 1 Destroy the Lock ( Execute this step in window 1 )\n");
				Print_Stmt("\tSTEP 5 > Make machine 2 also to Destroy the Lock ( Execute this step in window 2 )\n");
				Print_Stmt("\tSTEP 6 > Make machine 1 Release the Lock to actually Destroy the Lock from server memory( Execute this step in window 1 )\n");
				Print_Stmt("\n\nEnter the STEP no. you want to run here ->");
				
				userInput = Scan("%d");
				
				switch(userInput){
		
					case 1:		Destroy_Lock_W1_Create();
						break;
					case 2:		Destroy_Lock_W2_Create();
						break;	
					case 3:		Destroy_Lock_W1_Acquire();
						break;
					case 4:		Destroy_Lock_W1_Destroy();
						break;
					case 5:		Destroy_Lock_W2_Destroy();
						break;
					case 6:		Destroy_Lock_W1_Release();
						break;		
					default: Write("\n Invalid STEP entered by the user !!\n\n", 41, ConsoleOutput);	
						break;	
				}
				break;
		case 5:		Fork(CreateCV_RPC_test);
				break;
	 	case 6:	
				Print_Stmt("\tSTEP 1 > Make this machine go on WAIT\n");
				Print_Stmt("\tSTEP 2 > Make this machine SIGNAL the waiting machine\n");
				Print_Stmt("\n\nEnter the STEP no. you want to run here ->");
				userInput = Scan("%d");
				
				switch(userInput){
		
				case 1:		Fork(WaitCV_RPC_test);
					break;
				case 2:		Fork(SignalCV_RPC_test);
					break;	
				default: Write("\n Invalid STEP entered by the user !!\n\n", 41, ConsoleOutput);	
					break;	
				}
				break;
				
		case 7:	
				Print_Stmt("\nExecute the following STEPS one by one in different windows running different machines\n");
				Print_Stmt("\tSTEP 1 > Make machine 1 go on WAIT\n");
				Print_Stmt("\tSTEP 2 > Make machine 2 go on WAIT\n");
				Print_Stmt("\tSTEP 3 > Make machine 3 go on WAIT\n");
				Print_Stmt("\tSTEP 4 > Make this machine BROADCAST the waiting machines\n");
				Print_Stmt("\n\nEnter the STEP no. you want to run here ->");
				userInput = Scan("%d");
				
				switch(userInput){
		
				case 1:		Fork(BroadcastCV_Wait_RPC_test);
					break;
				case 2:		Fork(BroadcastCV_Wait_RPC_test);
					break;	
				case 3:		Fork(BroadcastCV_Wait_RPC_test);
					break;	
				case 4:		Fork(BroadcastCV_RPC_test);;
					break;	
				default: Write("\n Invalid STEP entered by the user !!\n\n", 41, ConsoleOutput);	
					break;	
				}
				break;
				
		case 8:		
				Print_Stmt("\tSTEP 1 > Make this machine go on Wait before Destroy CV\n");
				Print_Stmt("\tSTEP 2 > Make this machine Destroy CV and signal the waiting machine\n");
				Print_Stmt("\n\nEnter the STEP no. you want to run here ->");
				userInput = Scan("%d");
				
				switch(userInput){
		
				case 1:		Fork(DestroyCV_Wait_RPC_test);
					break;
				case 2:		Fork(DestroyCV_Signal_RPC_test);
					break;	
				default: Write("\n Invalid STEP entered by the user !!\n\n", 41, ConsoleOutput);			
					break;
				}
				break;
				
		case 9:		Fork(CreateMV_RPC_test);
				break;
				
		case 10:	Fork(GetMV_RPC_test);
				break;

		case 11:	Fork(SetMV_RPC_test);
				break;
		
		case 12:
				Print_Stmt("\nNOTE TO THE GRADER : \nPlease restart the server and client for executing this test case\n");
				Print_Stmt("Because to make this test case work the MV Index \n");
				Print_Stmt("of the MV to be destroyed is fixed to zero '0'\n");
				Print_Stmt("\nIf above note is followed then select the following steps one by one\n");
				
				Print_Stmt("\tSTEP 1 > Make machine 1 Create a MV ( Execute this step in window 1 )\n");
				Print_Stmt("\tSTEP 2 > Make machine 2 Create the same MV ( Execute this step in window 2 )\n");
				Print_Stmt("\tSTEP 3 > Make machine 1 Destroy the MV ( Execute this step in window 1 )\n");
				Print_Stmt("\tSTEP 4 > Make machine 2 also to Destroy the MV ( Execute this step in window 2 )\n");
				Print_Stmt("\n\nEnter the STEP no. you want to run here ->");
				
				userInput = Scan("%d");
				
				switch(userInput){
		
					case 1:		Destroy_MV_W1_Create();
						break;
					case 2:		Destroy_MV_W2_Create();
						break;	
					case 3:		Destroy_MV_W1_Destroy();
						break;
					case 4:		Destroy_MV_W2_Destroy();
						break;
					default: Write("\n Invalid STEP entered by the user !!\n\n", 41, ConsoleOutput);	
						break;	
				}
				break;
				
		default: Write("\n Invalid option entered by the user !!\n\n", 41, ConsoleOutput);	
				break; 		
				
	}  	
	
/* 	Exit(0); */
}