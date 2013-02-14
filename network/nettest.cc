// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

#define LOCK_BUSY 0
#define LOCK_FREE 1

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

//********************************
//		SERVER LOCKS
//********************************
struct serverSecretLocks
{
	char *lockName;
	int machineID;
	int mailBoxNumber;
	int status;
	int toBeDestroyedCount;
	int usageCounter; 
};
serverSecretLocks locksDB[MAX_LOCKS];
int locksDBIndex = -1;

//********************************
//		SERVER CVS
//********************************
struct serverSecretCVs
{
	char *cvName;
	int machineID;
	int mailBoxNumber;
	int cvLockIndex;
	int toBeDestroyedCount;
	int usageCounter; 
};
serverSecretCVs cvsDB[MAX_CVS];
int cvsDBIndex = -1;

//********************************
//		SERVER MVS
//********************************
struct serverSecretMVs
{
	char *mvName;
	int toBeDestroyedCount;
	int value;
};
serverSecretMVs mvsDB[MAX_MVS];
int mvsDBIndex = -1;

//********************************
//		LOCK WAIT ENTRY
//********************************
struct lockWaitEntry
{
	char *replymessage;
	PacketHeader replyOutPktHdr;
	MailHeader replyOutMailHdr;
};
List *lockqueue[MAX_LOCKS];

//********************************
//		CV WAIT ENTRY
//********************************
struct cvWaitEntry
{
	char *replymessage;
	PacketHeader replyOutPktHdr;
	MailHeader replyOutMailHdr;
};
List *cvqueue[MAX_CVS];

cvWaitEntry* removedCVEntry[MAX_CVS];
lockWaitEntry *lockEntry[MAX_LOCKS];
lockWaitEntry *removedLockEntry[MAX_LOCKS];
cvWaitEntry *CVEntry[MAX_CVS];

int lockEntryCounter = -1;
int removedLockEntryCounter = -1;
int cvEntryCounter = -1;
int removedCVEntryCounter = -1;

bool broadcastBit = false;	

//*********************************************************************************************
//									CREATE LOCK
//	name - Name of the Lock to be created on the server
//	senderMachineID - Owner machine of the Lock that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the Lock that is going to be created			
//*********************************************************************************************
void CreateLockRPC(char *name, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	char *failureMessage = NULL;
	
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	bool success = 0;
	
//------------------------------------------------------------------------------------------
// 							Validate the Lock parameters received
//------------------------------------------------------------------------------------------
	
	for(int index = 0; index <= locksDBIndex; index++){
		if(locksDB[index].toBeDestroyedCount != 0){
			if(strcmp(locksDB[index].lockName, name) == 0){
				
				// Increment the number of creators of the Lock
				locksDB[index].toBeDestroyedCount++;
				printf("CreateLockRPC : Lock already exists with the same name %s\n", name);
				
				sprintf(message, "%d : Lock already exists", index);
				// Format the reply message to the client waiting for the Lock Index from the server
				// Sending the lock index of the lock which is already created by someother client
				outMailHdr.length = strlen(message) + 1;
				// $
				printf("CreateLockRPC : reply message is -> %s\n", message);
				// Send the message
				success = postOffice->Send(outPktHdr, outMailHdr, message); 

				if ( !success ) {
					printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}	
				
				return;
			}
		}	
	}
	
	if(locksDBIndex == (MAX_LOCKS-1)){
		printf("CreateLockRPC : Reached max Lock Count, No more locks can be created\n");	
		
		failureMessage = "FAILURE : Reached max Lock Count";
			// Format the FAILURE reply message to the client waiting for the Lock Index from the server
			sprintf(message, "%s", failureMessage);
			outMailHdr.length = strlen(message) + 1;
			// $
			printf("CreateLockRPC : reply message is -> %s\n", message);
			// Send the message
			success = postOffice->Send(outPktHdr, outMailHdr, message); 

			if ( !success ) {
				printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}	
	
			return;
	}
	
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------

	// Move the index by one to store the information of the new lock created by the server
	locksDBIndex++;
	
	// Store the Lock information
	strcpy(locksDB[locksDBIndex].lockName, name);
	// toBeDestroyed flag will be set when the destroy lock is called
	locksDB[locksDBIndex].toBeDestroyedCount = 1;
	
	// used to keep track of number of waiters waiting to acquire the Lock
	// when this value becomes zero when the last thread calls release then the 
	// toBeDestroyd flag is checked for 1. If 1 then lock is destroyed 
	locksDB[locksDBIndex].usageCounter = 0;
	locksDB[locksDBIndex].machineID = -1;
	locksDB[locksDBIndex].mailBoxNumber = -1;
	locksDB[locksDBIndex].status = LOCK_FREE;
	
//------------------------------------------------------------------------------------------
//							Form a reply message and send to client
//------------------------------------------------------------------------------------------
	
	// Format the reply message to the client waiting for the Lock Index from the server
	sprintf(message, "%d ", locksDBIndex);
	outMailHdr.length = strlen(message) + 1;
	// $
	printf("CreateLockRPC : reply message is -> %s\n", message);
    // Send the message
    success = postOffice->Send(outPktHdr, outMailHdr, message); 

    if ( !success ) {
		printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
    }
	
	return;
}


//*********************************************************************************************
//									ACQUIRE LOCK
//	lockIndex - Index of the Lock to be acquired
//	senderMachineID - machine id of the client that is requesting for acquire
//	senderMailBoxNumber - machine mail box number of the client that is requesting for acquire
//*********************************************************************************************

void AcquireLockRPC(int lockIndex, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	char sendMessage[MaxMailSize]; 
	char receiveMessage[MaxMailSize]; 
	bool success = false;
	bool failure = false;
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	
	//if Invalid lock index
	if(locksDB[lockIndex].toBeDestroyedCount == 0){
		sprintf(message, "FAILURE : Lock %d is Destroyed" ,lockIndex);
		failure = true;
	}
	else if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d",lockIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : lock %d does not exist",lockIndex);
		failure = true;
	}
	else if((senderMailBoxNumber == locksDB[lockIndex].mailBoxNumber) && (senderMachineID == locksDB[lockIndex].machineID)){
		//	This lock is held by the current thread
		sprintf(message, "%d : Is owner of Lock %d",lockIndex, lockIndex);
		failure = true;
	}// If the Lock is destroyed then its not possible to acquire it
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("AcquireLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
	if(locksDB[lockIndex].status == LOCK_FREE){
		// Lock is free and can be aquired
		// Make the Lock status BUSY and make the current thread as the owner
		locksDB[lockIndex].status = LOCK_BUSY;	
		locksDB[lockIndex].machineID = senderMachineID;
		locksDB[lockIndex].mailBoxNumber = senderMailBoxNumber;
		printf("AcquireLockRPC : Lock %d acquired by machine %d from mail Box Number %d\n", lockIndex, senderMachineID, senderMailBoxNumber);
		// Update the usage counter to keep track of number of users for the lock
		locksDB[lockIndex].usageCounter ++;
		sprintf(message, "%d : Lock %d is Acquired",lockIndex, lockIndex);
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("AcquireLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}
	else{
		// Lock is busy and have to wait 
		// Make space in the queue for the reply packed to be stored
		sprintf(message, "%d : Lock %d is Acquired",lockIndex, lockIndex);
		// Update the usage counter to keep track of number of users for the lock 
		locksDB[lockIndex].usageCounter ++;
		// Hav a count of the number of packets being stored into the queue to use the new free structure in the array of structures
		lockEntryCounter++;
		strcpy((*lockEntry[lockEntryCounter]).replymessage, message);
		(*lockEntry[lockEntryCounter]).replyOutPktHdr = 	outPktHdr;
		(*lockEntry[lockEntryCounter]).replyOutMailHdr = outMailHdr;
	
		printf("AcquireLockRPC : Thread on machine %d with mailbox number %d is going on wait for the Lock %d\n", outPktHdr.to, outMailHdr.to, lockIndex);
		// Append the send packet to the queue
		lockqueue[lockIndex]->Append((void *)lockEntry[lockEntryCounter]);
	}
	return;
}


//*********************************************************************************************
//									RELEASE LOCK
//	lockIndex - Index of the Lock to be released
//	senderMachineID - machine id of the client that is requesting for release
//	senderMailBoxNumber - machine mail box number of the client that is requesting for release
//*********************************************************************************************
void ReleaseLockRPC(int lockIndex, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	
	//if Invalid lock index
	if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d",lockIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : Lock %d does not exist",lockIndex);
		failure = true;
	}
	else if(senderMailBoxNumber != locksDB[lockIndex].mailBoxNumber){
		//	This lock is not held by the current thread
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if(senderMachineID != locksDB[lockIndex].machineID){
		//	This lock is not held by the current thread
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("ReleaseLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
	if(!lockqueue[lockIndex]->IsEmpty()){
	
		sprintf(message, "%d : Lock %d is Released",lockIndex, lockIndex);
		outMailHdr.length = strlen(message) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, message); 
	
		if (!success ) {
			printf("ReleaseLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		
		// Remove a send packet from the queue
		removedLockEntryCounter++;
		removedLockEntry[removedLockEntryCounter] = (lockWaitEntry*)lockqueue[lockIndex]->Remove();
		if(removedLockEntry[removedLockEntryCounter] != NULL){	// Reply to the removed thread
			printf("ReleaseLockRPC : Lock %d released by machine %d from mail Box Number %d\n", lockIndex, senderMachineID, senderMailBoxNumber);
			
			outPktHdr = (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr;
			outMailHdr = (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr;	
			strcpy(message, (*removedLockEntry[removedLockEntryCounter]).replymessage);
			outMailHdr.length = strlen(message) + 1;
			sprintf(message, "%d : Lock %d is Acquired",lockIndex, lockIndex);
			
			// Send the reply message
			success = postOffice->Send(outPktHdr, outMailHdr, message); 

			if (!success ) {
				printf("ReleaseLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		// Make the Lock status BUSY and make the current thread as the owner
		locksDB[lockIndex].status = LOCK_BUSY;	
		locksDB[lockIndex].machineID = (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr.to;
		locksDB[lockIndex].mailBoxNumber = (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr.to;
		// Update the usage counter to keep track of number of users for the lock 
		locksDB[lockIndex].usageCounter --;
		printf("ReleaseLockRPC : Lock %d acquired by machine %d from mail Box Number %d\n", lockIndex, (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr.to, (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr.to);	
		
	}	
	else{
		
		// Queue is empty so Lock is made free
		locksDB[lockIndex].status = LOCK_FREE;
		locksDB[lockIndex].machineID = -1;
		locksDB[lockIndex].mailBoxNumber = -1;
		// Update the usage counter to keep track of number of users for the lock 
		locksDB[lockIndex].usageCounter --;
		
		if((locksDB[lockIndex].usageCounter == 0) && (locksDB[lockIndex].toBeDestroyedCount == 0))
		{
			delete locksDB[lockIndex].lockName;
			locksDB[lockIndex].lockName = NULL;
			locksDB[lockIndex].machineID = -1;
			locksDB[lockIndex].mailBoxNumber = -1;
			locksDB[lockIndex].status = LOCK_FREE;
			locksDB[lockIndex].toBeDestroyedCount = 0;
			printf("ReleaseLockRPC : Lock %d is Destroyed in release syscall\n",lockIndex);
		}
		
		sprintf(message, "%d : Lock %d is Released",lockIndex, lockIndex);
		outMailHdr.length = strlen(message) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if (!success ) {
			printf("ReleaseLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}

	}
	return;
}
 
//*********************************************************************************************
//									DESTROY LOCK
//	lockIndex - Index of the Lock to be destroy
//	senderMachineID - machine id of the client that is requesting for destroy
//	senderMailBoxNumber - machine mail box number of the client that is requesting for destroy
//*********************************************************************************************

void DestroyLockRPC(int lockIndex, int senderMachineID, int senderMailBoxNumber) {
	
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	//if Invalid lock index
	if(locksDB[lockIndex].toBeDestroyedCount == 0){
		//	This lock is not held by the current thread
		sprintf(message, "FAILURE : Lock %d already destroyed",lockIndex);
		failure = true;
	}
	else if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d\n",lockIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : Lock %d does not exist",lockIndex);
		failure = true;
	}
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("DestroyLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------
	if(locksDB[lockIndex].toBeDestroyedCount != 0){
		locksDB[lockIndex].toBeDestroyedCount--; 
	}

	if((locksDB[lockIndex].usageCounter == 0) && (locksDB[lockIndex].toBeDestroyedCount == 0)){ 
		// Destroy the Lock right now
		delete locksDB[lockIndex].lockName;
		locksDB[lockIndex].lockName = NULL;
		locksDB[lockIndex].machineID = -1;
		locksDB[lockIndex].mailBoxNumber = -1;
		locksDB[lockIndex].status = LOCK_FREE;
		locksDB[lockIndex].toBeDestroyedCount = 0;
		sprintf(message, "%d : Lock %d is Destroyed",lockIndex, lockIndex);
		printf("DestroyLockRPC : Lock %d Destroyed by machine %d from mail Box Number %d\n", lockIndex, senderMachineID, senderMailBoxNumber);
	}
	
	// Following if statements can be replaced with a single else statement but to make it more sensible 3 if statements are placed below
	if((locksDB[lockIndex].usageCounter != 0) && (locksDB[lockIndex].toBeDestroyedCount == 0)){
	
		sprintf(message, "%d : Lock %d is going to be Destroyed",lockIndex, lockIndex);
	}	
	
	if((locksDB[lockIndex].usageCounter == 0) && (locksDB[lockIndex].toBeDestroyedCount != 0)){
	
		sprintf(message, "%d : Lock %d is going to be Destroyed",lockIndex, lockIndex);
	}	
	
	if((locksDB[lockIndex].usageCounter != 0) && (locksDB[lockIndex].toBeDestroyedCount != 0)){
	
		sprintf(message, "%d : Lock %d is going to be Destroyed",lockIndex, lockIndex);
	}	
	
	outMailHdr.length = strlen(message) + 1;
	success = postOffice->Send(outPktHdr, outMailHdr, message); 
	if (!success ) {
		printf("DestroyLockRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}

	return;
}

 
//*********************************************************************************************
//									CREATE CONDITION VARIABLE
//	name - Name of the CV to be created on the server
//	senderMachineID - Owner machine of the CV that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the CV that is going to be created			
//*********************************************************************************************
void CreateCVRPC(char *name, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	bool success = 0;
	printf("CV name requested %s\n", name);
//------------------------------------------------------------------------------------------
// 							Validate the CV parameters received
//------------------------------------------------------------------------------------------
	if(cvsDB[0].cvName == NULL)
		printf("stored is NULL\n");
		printf("cvsDBIndex is %d\n", cvsDBIndex);
	for(int index = 0; index <= cvsDBIndex; index++){
		if(cvsDB[index].toBeDestroyedCount != 0){
			if(strcmp(cvsDB[index].cvName, name) == 0){
				printf("CreateCVRPC : CV already exists with the same name %s\n", name);
				
				// increment the number of creators of the CV with same name before being destroyed
				cvsDB[index].toBeDestroyedCount++;
				
				sprintf(message, "%d : CV already exists", index);
				// Format the reply message to the client waiting for the CV Index from the server
				// Sending the CV index of the CV which is already created by someother client
				outMailHdr.length = strlen(message) + 1;
				// $
				printf("CreateCVRPC : reply message is -> %s\n", message);
				// Send the message
				success = postOffice->Send(outPktHdr, outMailHdr, message); 

				if ( !success ) {
					printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}	
				
				return;
			}
		}	
	}
	if(cvsDBIndex == (MAX_CVS-1)){
		printf("CreateCVRPC : Reached max CV Count, No more CVs can be created\n");	
		
		failureMessage = "FAILURE : Reached max CV Count";
		// Format the FAILURE reply message to the client waiting for the CV Index from the server
		sprintf(message, "%s", failureMessage);
		outMailHdr.length = strlen(message) + 1;
		// $
		printf("CreateCVRPC : reply message is -> %s\n", message);
		// Send the message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}	
		return;
	}
	
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------

	// Move the index by one to store the information of the new CV created by the server
	cvsDBIndex++;
	// Store the Lock information
	strcpy(cvsDB[cvsDBIndex].cvName, name);
	// toBeDestroyed flag will be set when the destroy lock is called
	cvsDB[cvsDBIndex].toBeDestroyedCount = 1;
	// used to keep track of number of waiters waiting to acquire the Lock
	// when this value becomes zero when the last thread calls release then the 
	// toBeDestroyd flag is checked for 1. If 1 then lock is destroyed 
	cvsDB[cvsDBIndex].usageCounter = 0;
	cvsDB[cvsDBIndex].machineID = senderMachineID;
	cvsDB[cvsDBIndex].mailBoxNumber = senderMailBoxNumber;
	cvsDB[cvsDBIndex].cvLockIndex = -1;
//------------------------------------------------------------------------------------------
//							Form a reply message and send to client
//------------------------------------------------------------------------------------------

	// Format the reply message to the client waiting for the CV Index from the server
	sprintf(message, "%d ", cvsDBIndex);
	outMailHdr.length = strlen(message) + 1;
	// $
	printf("CreateCVRPC : reply message is -> %s\n", message);
    // Send the first message
    success = postOffice->Send(outPktHdr, outMailHdr, message); 
    if ( !success ) {
		printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
    }
	
	return;
}

 
//*********************************************************************************************
//									CONDITION WAIT 
//	cvIndex - Index of the CV for this wait
//	lockIndex - Index of the Lock for this Condition
//	senderMachineID - machine id of the client that is requesting for CVwait
//	senderMailBoxNumber - machine mail box number of the client that is requesting for CVwait
//*********************************************************************************************
 
void CVWaitRPC(int cvIndex, int lockIndex, int senderMachineID, int senderMailBoxNumber){
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d\n",lockIndex);
		failure = true;
	}	
	else if((cvIndex < 0)||(cvIndex > cvsDBIndex)){
		sprintf(message, "FAILURE : Invalid CV index %d\n",cvIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : Lockname %d does not exist",lockIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].cvName == NULL){
		sprintf(message, "FAILURE : CVname %d does not exist",cvIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].toBeDestroyedCount == 0){
		sprintf(message, "FAILURE : CV %d is Destroyed" ,cvIndex);
		failure = true;
	}
	else if(senderMailBoxNumber != locksDB[lockIndex].mailBoxNumber){
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if(senderMachineID != locksDB[lockIndex].machineID){
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].cvLockIndex != lockIndex){
			if(cvsDB[cvIndex].cvLockIndex != -1) {
				sprintf(message, "FAILURE : CVlock %d mismatch with Lock %d",cvsDB[cvIndex].cvLockIndex, lockIndex);
				failure = true;
			}
	}	
	
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("CVWaitRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------
	// If CV has not waiters yet then make the current Lock as the CV waiting lock
	if (cvsDB[cvIndex].cvLockIndex == -1){
		cvsDB[cvIndex].cvLockIndex = lockIndex;
	}
	
	//Release the lock before going to wait on CV
	if(!lockqueue[lockIndex]->IsEmpty()){
		removedLockEntryCounter++;
		removedLockEntry[removedLockEntryCounter] = (lockWaitEntry*)lockqueue[lockIndex]->Remove();
		if(removedLockEntry[removedLockEntryCounter] != NULL){	
			printf("CVWaitRPC : Lock %d released by machine %d from mail Box Number %d\n", lockIndex, senderMachineID, senderMailBoxNumber);
			outPktHdr = (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr;
			outMailHdr = (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr;	
			strcpy(message, (*removedLockEntry[removedLockEntryCounter]).replymessage);
			outMailHdr.length = strlen(message) + 1;
			// Send the reply message
			success = postOffice->Send(outPktHdr, outMailHdr, message); 
			if (!success ) {
				printf("CVWaitRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
		locksDB[lockIndex].status = LOCK_BUSY;	
		locksDB[lockIndex].machineID = (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr.to;
		locksDB[lockIndex].mailBoxNumber = (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr.to;
		printf("CVWaitRPC : Lock %d acquired by machine %d from mail Box Number %d\n", lockIndex, (*removedLockEntry[removedLockEntryCounter]).replyOutPktHdr.to, (*removedLockEntry[removedLockEntryCounter]).replyOutMailHdr.to);	
	}
	else{
		// If no waiters for the Lock being released then make the Lock free
		locksDB[lockIndex].status = LOCK_FREE;
		locksDB[lockIndex].machineID = -1;
		locksDB[lockIndex].mailBoxNumber = -1;
		printf("CVWaitRPC : Lock %d is made FREE by machine %d mail Box No.%d since no waiters for lock\n", lockIndex, senderMachineID, senderMailBoxNumber);	
	}
	printf("CVWaitRPC : Machine %d from mail box number %d going on WAIT in CV %d and Lock %d\n", senderMachineID, senderMailBoxNumber, cvIndex, lockIndex);	
	//Append this one to cvqueue - Client replied when a signal is received for this CV on this Lock
	//Store the message to be printed after receiving the signal from a thread
	
	// Update the waiter count of the CV
	cvsDB[cvIndex].usageCounter ++;
	
	
	sprintf(message,"%d : client out of CV Q %d", cvIndex, cvIndex);
	cvEntryCounter++;
	strcpy((*CVEntry[cvEntryCounter]).replymessage, message);
	(*CVEntry[cvEntryCounter]).replyOutPktHdr = outPktHdr;
	(*CVEntry[cvEntryCounter]).replyOutMailHdr = outMailHdr;	
	cvqueue[cvIndex]->Append((void *)CVEntry[cvEntryCounter]);
	return;
}


//*********************************************************************************************
//									CONDITION SIGNAL 
//	cvIndex - Index of the CV for this signal
//	lockIndex - Index of the Lock for this Condition
//	senderMachineID - machine id of the client that is requesting for CVsignal
//	senderMailBoxNumber - machine mail box number of the client that is requesting for CVsignal
//*********************************************************************************************
 
void CVSignalRPC(int cvIndex, int lockIndex, int senderMachineID, int senderMailBoxNumber){
	
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d",lockIndex);
		failure = true;
	}	
	else if((cvIndex < 0)||(cvIndex > cvsDBIndex)){
		sprintf(message, "FAILURE : Invalid CV index %d",lockIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : Lockname %d does not exist",lockIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].cvName == NULL){
		sprintf(message, "FAILURE : CVname %d does not exist",lockIndex);
		failure = true;
	}
	else if(senderMailBoxNumber != locksDB[lockIndex].mailBoxNumber){
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if(senderMachineID != locksDB[lockIndex].machineID){
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if ((cvsDB[cvIndex].cvLockIndex != lockIndex) && (!failure)) {
		sprintf(message, "FAILURE : CVlock %d mismatch with Lock %d",cvsDB[cvIndex].cvLockIndex, lockIndex);
		failure = true;
	}	
	
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 
		if ( !success ) {
			printf("CVsignalRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------
	if(cvqueue[cvIndex]->IsEmpty()){
		sprintf(message, "%d : No waiters in CV %d wait queue", cvIndex, cvIndex);
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 
		if ( !success ) {
			printf("CVsignalRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	else {
		removedCVEntryCounter++;
	 	removedCVEntry[removedCVEntryCounter] = (cvWaitEntry*)cvqueue[cvIndex]->Remove();
		
		cvsDB[cvIndex].usageCounter --;
		if(removedCVEntry[removedCVEntryCounter] != NULL){
			//Acquire the lock
			if(locksDB[lockIndex].status == LOCK_FREE){
				locksDB[lockIndex].status = LOCK_BUSY;	
				locksDB[lockIndex].machineID = (*removedCVEntry[removedCVEntryCounter]).replyOutPktHdr.to;
				locksDB[lockIndex].mailBoxNumber = (*removedCVEntry[removedCVEntryCounter]).replyOutMailHdr.to;
				printf("CVsignalRPC : Lock %d acquired by machine %d from mail Box Number %d\n", lockIndex, (*removedCVEntry[removedCVEntryCounter]).replyOutPktHdr.to, (*removedCVEntry[removedCVEntryCounter]).replyOutMailHdr.to);
				strcpy(message,(*removedCVEntry[removedCVEntryCounter]).replymessage);
				outMailHdr.length = strlen(message) + 1;
				// Send the reply message
				success = postOffice->Send((*removedCVEntry[removedCVEntryCounter]).replyOutPktHdr, (*removedCVEntry[removedCVEntryCounter]).replyOutMailHdr, message); 
				if ( !success ) {
					printf("CVsignalRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}
			}
			else{
				lockEntryCounter++;
				strcpy((*lockEntry[lockEntryCounter]).replymessage,(*removedCVEntry[removedCVEntryCounter]).replymessage);
				(*lockEntry[lockEntryCounter]).replyOutPktHdr = (*removedCVEntry[removedCVEntryCounter]).replyOutPktHdr;
				(*lockEntry[lockEntryCounter]).replyOutMailHdr = (*removedCVEntry[removedCVEntryCounter]).replyOutMailHdr;
				lockqueue[lockIndex]->Append((void *)lockEntry[lockEntryCounter]);
			}	
		} 
	}
	if(cvqueue[cvIndex]->IsEmpty()){
		cvsDB[cvIndex].cvLockIndex = -1;
	}
	
	if((cvsDB[cvIndex].usageCounter == 0) && (cvsDB[cvIndex].toBeDestroyedCount == 0)){ 
		//destroy the CV if no waiters on this CV and the destroy flag is set
		delete cvsDB[cvIndex].cvName;
		cvsDB[cvIndex].cvName = NULL;
		cvsDB[cvIndex].mailBoxNumber = -1;
		cvsDB[cvIndex].machineID = -1;
		cvsDB[cvIndex].toBeDestroyedCount = 0;
		printf("%d : CV %d is Destroyed by signal",cvIndex, cvIndex);
	}
	
	if(broadcastBit == false){
		sprintf(message, "%d : CV %d Signal completed with lock %d", cvIndex, cvIndex, lockIndex);
		outMailHdr.length = strlen(message) + 1;
		success = postOffice->Send(outPktHdr, outMailHdr, message); 
		if (!success ) {
			printf("CVsignalRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
	}
	
	return;
}
 

//*********************************************************************************************
//									CONDITION BROADCAST 
//	cvIndex - Index of the CV for this broadcast
//	lockIndex - Index of the Lock for this Condition
//	senderMachineID - machine id of the client that is requesting for CVbroadcast
//	senderMailBoxNumber - machine mail box number of the client that is requesting for CVbroadcast
//*********************************************************************************************
 

void CVBroadcastRPC(int cvIndex, int lockIndex, int senderMachineID, int senderMailBoxNumber){

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	char *failureMessage = NULL;

	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	if((lockIndex < 0)||(lockIndex > locksDBIndex)){
		sprintf(message, "FAILURE : Invalid Lock index %d\n",lockIndex);
		failure = true;
	}	
	else if((cvIndex < 0)||(cvIndex > cvsDBIndex)){
		sprintf(message, "FAILURE : Invalid CV index %d\n",lockIndex);
		failure = true;
	}	
	else if(locksDB[lockIndex].lockName == NULL){
		sprintf(message, "FAILURE : Lockname %d does not exist",lockIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].cvName == NULL){
		sprintf(message, "FAILURE : CVname %d does not exist",lockIndex);
		failure = true;
	}
	else if((senderMailBoxNumber != locksDB[lockIndex].mailBoxNumber)&(senderMachineID != locksDB[lockIndex].machineID)){
		sprintf(message, "FAILURE : Not Lock %d owner",lockIndex);
		failure = true;
	}
	else if (cvsDB[cvIndex].cvLockIndex != lockIndex) {
		sprintf(message, "FAILURE : CVlock %d mismatch with %d",cvsDB[cvIndex].cvLockIndex, lockIndex);
		failure = true;
	}	
	
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 
		if ( !success ) {
			printf("CVbroadcastRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------

	while ((cvqueue[cvIndex]->IsEmpty()) == FALSE){
    	broadcastBit = true;    
		CVSignalRPC(cvIndex, lockIndex, senderMachineID, senderMailBoxNumber);

	}
	
	// Update the waiter count of the CV
	cvsDB[cvIndex].usageCounter = 0;
	
	broadcastBit = false;   
	
	if((cvsDB[cvIndex].usageCounter == 0) && (cvsDB[cvIndex].toBeDestroyedCount == 0)){ 
		//destroy the CV if no waiters on this CV and the destroy flag is set
		delete cvsDB[cvIndex].cvName;
		cvsDB[cvIndex].cvName = NULL;
		cvsDB[cvIndex].mailBoxNumber = -1;
		cvsDB[cvIndex].machineID = -1;
		cvsDB[cvIndex].toBeDestroyedCount = 0;
		printf("%d : CV %d is Destroyed by Broadcast",cvIndex, cvIndex);
	}
	
	sprintf(message, "%d : CV %d finish Broadcast with lock %d",cvIndex, cvIndex, lockIndex);
	outMailHdr.length = strlen(message) + 1;
	success = postOffice->Send(outPktHdr, outMailHdr, message); 
	if (!success ) {
		printf("CVbroadcastRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
	return;

}

 
//*********************************************************************************************
//									CONDITION DESTROY 
//	cvIndex - Index of the CV for this destroy
//	senderMachineID - machine id of the client that is requesting for destroy
//	senderMailBoxNumber - machine mail box number of the client that is requesting for destroy
//*********************************************************************************************

void CVDestroyRPC(int cvIndex, int senderMachineID, int senderMailBoxNumber){
	
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	bool success = false;
	bool failure = false;
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	
	if(cvsDB[cvIndex].toBeDestroyedCount == 0){
		//	This lock is not held by the current thread
		sprintf(message, "FAILURE : CV %d already destroyed",cvIndex);
		failure = true;
	}
	else if((cvIndex < 0)||(cvIndex > cvsDBIndex)){
		//if Invalid lock index
		sprintf(message, "FAILURE : Invalid CV index %d\n",cvIndex);
		failure = true;
	}	
	else if(cvsDB[cvIndex].cvName == NULL){
		sprintf(message, "FAILURE : CV %d does not exist",cvIndex);
		failure = true;
	}
	
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("DestroyCVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------
	
	if(cvsDB[cvIndex].toBeDestroyedCount != 0){
		
		cvsDB[cvIndex].toBeDestroyedCount--;
		
	}
	
	if((cvsDB[cvIndex].usageCounter == 0) && (cvsDB[cvIndex].toBeDestroyedCount == 0)){ 
		//destroy
		delete cvsDB[cvIndex].cvName;
		cvsDB[cvIndex].cvName = NULL;
		cvsDB[cvIndex].cvLockIndex = -1;
		cvsDB[cvIndex].mailBoxNumber = -1;
		cvsDB[cvIndex].machineID = -1;
		cvsDB[cvIndex].toBeDestroyedCount = 0;
		sprintf(message, "%d : CV %d is Destroyed",cvIndex, cvIndex);
		printf("DestroyCVRPC : CV %d Destroyed by machine %d from mail Box Number %d\n", cvIndex, senderMachineID, senderMailBoxNumber);
	}
	
	// Following if statements can be replaced with a single else statement but to make it more sensible 3 if statements are placed below
	if((cvsDB[cvIndex].usageCounter != 0) && (cvsDB[cvIndex].toBeDestroyedCount == 0)){
		sprintf(message, "%d : CV %d is going to be Destroyed",cvIndex, cvIndex);
	}	
	
	if((cvsDB[cvIndex].usageCounter == 0) && (cvsDB[cvIndex].toBeDestroyedCount != 0)){
		sprintf(message, "%d : CV %d is going to be Destroyed",cvIndex, cvIndex);
	}	
	
	if((cvsDB[cvIndex].usageCounter != 0) && (cvsDB[cvIndex].toBeDestroyedCount != 0)){
		sprintf(message, "%d : CV %d is going to be Destroyed",cvIndex, cvIndex);
	}	
	
	outMailHdr.length = strlen(message) + 1;
	success = postOffice->Send(outPktHdr, outMailHdr, message); 
	if (!success ) {
		printf("DestroyCVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}
	return;
	
}


//*********************************************************************************************
//									CREATE MONITOR VARIABLE
//	name - Name of the MV to be created on the server
//	senderMachineID - Owner machine of the MV that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the MV that is going to be created			
//*********************************************************************************************
void CreateMVRPC(char *name, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	char *failureMessage = new char[MaxMailSize];
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	bool success = 0;
	
//------------------------------------------------------------------------------------------
// 							Validate the MV parameters received
//------------------------------------------------------------------------------------------
	
	for(int index = 0; index <= mvsDBIndex; index++){
		if(mvsDB[index].toBeDestroyedCount != 0) {
			if(strcmp(mvsDB[index].mvName, name) == 0){
				printf("CreateMVRPC : MV already exists with the same name %s\n", name);
				
				sprintf(message, "%d : MV already exists", index);
				
				// increment the number of creators of the MV with same name before being destroyed
				mvsDB[index].toBeDestroyedCount++;
				
				// Format the reply message to the client waiting for the MV Index from the server
				// Sending the MV index of the MV which is already created by someother client
				outMailHdr.length = strlen(message) + 1;
				// $
				printf("CreateMVRPC : reply message is -> %s\n", message);
				// Send the message
				success = postOffice->Send(outPktHdr, outMailHdr, message); 

				if ( !success ) {
					printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
					interrupt->Halt();
				}	
				return;
			}
		}	
	}
	
	if(mvsDBIndex == (MAX_MVS-1)){
		printf("CreateMVRPC : Reached max MV Count, No more MVs can be created\n");	
		
		failureMessage = "FAILURE : Reached max MV Count";
		// Format the FAILURE reply message to the client waiting for the MV Index from the server
		sprintf(message, "%s", failureMessage);
		outMailHdr.length = strlen(message) + 1;
		// $
		printf("CreateMVRPC : reply message is -> %s\n", message);
		// Send the message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}	
		return;
	}
	
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------

	// Move the index by one to store the information of the new MV created by the server
	mvsDBIndex++;
	
	// Store the Lock information
	strcpy(mvsDB[mvsDBIndex].mvName, name);
	
	// toBeDestroyed counter will be decremented when the destroy MV is called
	mvsDB[mvsDBIndex].toBeDestroyedCount = 1;
	
	mvsDB[mvsDBIndex].value = 0;
	
//------------------------------------------------------------------------------------------
//							Form a reply message and send to client
//------------------------------------------------------------------------------------------

	// Format the reply message to the client waiting for the MV Index from the server
	sprintf(message, "%d ", mvsDBIndex);
	outMailHdr.length = strlen(message) + 1;
	// $
	printf("CreateMVRPC : reply message is -> %s\n", message);
    // Send the first message
    success = postOffice->Send(outPktHdr, outMailHdr, message); 

    if ( !success ) {
		printf("CreateMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
    }
	
	return;
}


//*********************************************************************************************
//									GET MONITOR VARIABLE
//	MVIndex - Index of the MV whose value is requested
//	senderMachineID - Owner machine of the MV that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the MV that is going to be created			
//*********************************************************************************************
void GetMVRPC(int MVIndex, int senderMachineID, int senderMailBoxNumber)  {
	
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	char sendMessage[MaxMailSize]; 
	char receiveMessage[MaxMailSize]; 
	bool success = false;
	bool failure = false;
	
	char *failureMessage = NULL;
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	
	//MV is already destroyed
	if(mvsDB[MVIndex].toBeDestroyedCount == 0){
		sprintf(message, "FAILURE : MV %d is Destroyed" ,MVIndex);
		failure = true;
	}
	else if((MVIndex < 0)||(MVIndex > mvsDBIndex)){
		sprintf(message, "FAILURE : Invalid MV index %d",MVIndex);
		failure = true;
	}	
	else if(mvsDB[MVIndex].mvName == NULL){
		sprintf(message, "FAILURE : MV %d name is NULL",MVIndex);
		failure = true;
	}
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("GetMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
	// Send back the value of the MV requested by the userprog
	sprintf(message, "%d : MV %d value is %d", mvsDB[MVIndex].value, MVIndex, mvsDB[MVIndex].value);
	outMailHdr.length = strlen(message) + 1;
	// Send the reply message
	success = postOffice->Send(outPktHdr, outMailHdr, message); 

	if ( !success ) {
		printf("GetMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}

	return;
}


//*********************************************************************************************
//									SET MONITOR VARIABLE
//	MVIndex - Index of the MV whose value is to be set
//	senderMachineID - Owner machine of the MV that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the MV that is going to be created			
//*********************************************************************************************
void SetMVRPC(int MVIndex, int MVValue, int senderMachineID, int senderMailBoxNumber)  {
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	char sendMessage[MaxMailSize]; 
	char receiveMessage[MaxMailSize]; 
	bool success = false;
	bool failure = false;
	
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
//------------------------------------------------------------------------------------------
// 							Validate the parameters received
//------------------------------------------------------------------------------------------	
	
	//MV is already destroyed
	if(mvsDB[MVIndex].toBeDestroyedCount == 0){
		sprintf(message, "FAILURE : MV %d is Destroyed" ,MVIndex);
		failure = true;
	}
	else if((MVIndex < 0)||(MVIndex > mvsDBIndex)){
		sprintf(message, "FAILURE : Invalid MV index %d",MVIndex);
		failure = true;
	}	
	else if(mvsDB[MVIndex].mvName == NULL){
		sprintf(message, "FAILURE : MV %d name is NULL",MVIndex);
		failure = true;
	}
	
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("SetMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
	// Set the value of the MV requested by the userprog
	mvsDB[MVIndex].value = MVValue;
	sprintf(message, "%d : MV %d value is set to %d",MVValue, MVIndex, MVValue);
	outMailHdr.length = strlen(message) + 1;
	// Send the reply message
	success = postOffice->Send(outPktHdr, outMailHdr, message); 
	if ( !success ) {
		printf("SetMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
	}

	return;
}


//*********************************************************************************************
//									DESTROY MONITOR VARIABLE
//	MVIndex - Index of the MV to be destroyed on the server
//	senderMachineID - Owner machine of the MV that is going to be created
//	senderMailBoxNumber - Owner machine mail box number of the MV that is going to be created			
//*********************************************************************************************
void DestroyMVRPC(int MVIndex, int senderMachineID, int senderMailBoxNumber) {

	PacketHeader outPktHdr;
    MailHeader outMailHdr;
	char *message = new char[MaxMailSize];
	outPktHdr.to = senderMachineID;
	outMailHdr.to = senderMailBoxNumber;	
	outMailHdr.from = RECEIVE_MAILBOX;
	outPktHdr.from = serverMachineID;
	bool success = 0;
	bool failure = false;
	
//------------------------------------------------------------------------------------------
// 							Validate the MV parameters received
//------------------------------------------------------------------------------------------
	//MV is already destroyed
	if(mvsDB[MVIndex].toBeDestroyedCount == 0){
		sprintf(message, "FAILURE : MV %d is already destroyed" ,MVIndex);
		failure = true;
	}
	else if((MVIndex < 0)||(MVIndex > mvsDBIndex)){
		sprintf(message, "FAILURE : Invalid MV index %d",MVIndex);
		failure = true;
	}	
	else if(mvsDB[MVIndex].mvName == NULL){
		sprintf(message, "FAILURE : MV %d name is NULL",MVIndex);
		failure = true;
	}
	if(failure){
		outMailHdr.length = strlen(message) + 1;
		// Send the reply message
		success = postOffice->Send(outPktHdr, outMailHdr, message); 

		if ( !success ) {
			printf("DestroyMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
			interrupt->Halt();
		}
		return;
	}
	
//------------------------------------------------------------------------------------------
//							Update Server secret storage	
//------------------------------------------------------------------------------------------
	
	// toBeDestroyed flag will is set so that no one else can GET or SET the value of the MV
	if(mvsDB[MVIndex].toBeDestroyedCount != 0){
		mvsDB[MVIndex].toBeDestroyedCount--;
	}
	mvsDB[MVIndex].value = 0;
	
//------------------------------------------------------------------------------------------
//							Form a reply message and send to client
//------------------------------------------------------------------------------------------
	
	// Format the reply message to the client waiting for the MV Index from the server
	if(mvsDB[MVIndex].toBeDestroyedCount == 0){
		sprintf(message, "%d : MV %d is Destroyed successfully", MVIndex, MVIndex);
	}
	else{
		sprintf(message, "%d : MV %d is going to be Destroyed", MVIndex, MVIndex);
	}
	outMailHdr.length = strlen(message) + 1;
	// $
	printf("DestroyMVRPC : reply message is -> %s\n", message);
    // Send the first message
    success = postOffice->Send(outPktHdr, outMailHdr, message); 

    if ( !success ) {
		printf("DestroyMVRPC : The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
    }
	
	return;
}


void ServerHeart() {
	
	char *rpcType = new char[30];
	char *name = new char[30];
	PacketHeader inPktHdr,outPktHdr;
	MailHeader inMailHdr,outMailHdr;
	char sendMessage[MaxMailSize]; 
	char receiveMessage[MaxMailSize]; 
	int type = 0;
	bool success = false;
	// Acquire variables
	char *acquireLockIndex = new char[10];
	char *releaseLockIndex = new char[10];
	int lockIndex = -1;		// Index of the Lock to be acquired
	int index = 0;
	// Wait variables
	char *strWaitLockIndex = new char[10];
	char *strWaitCVIndex = new char[10];
	int waitLockIndex = -1;
	int waitCVIndex = -1;
	// Signal variables
	char *strSignalLockIndex = new char[10];
	char *strSignalCVIndex = new char[10];
	int signalLockIndex = -1;
	int signalCVIndex = -1;
	// Broadcast variables
	char *strBroadcastLockIndex = new char[10];
	char *strBroadcastCVIndex = new char[10];
	int broadcastLockIndex = -1;
	int broadcastCVIndex = -1;
	// Destroy variables
	char *strDestroyLockIndex = new char[10];
	char *strDestroyCVIndex = new char[10];
	int destroyLockIndex = -1;
	int destroyCVIndex = -1;
	// Monitor 
	char *strMVIndex = new char[10];
	char *strMVValue = new char[10];
	int MVIndex = 0;
	int MVValue = 0;
	char *strDestroyMVIndex = new char[10];
	int destroyMVIndex = -1;
	
	
	// Structures used to store and fetch the packets from the lock/cv wait queues	
	for(index = 0; index < MAX_LOCKS; index++){
		lockqueue[index] = new List;
		locksDB[index].toBeDestroyedCount = 0;
		locksDB[index].lockName = new char[MAX_CHARLEN];
		lockEntry[index] = new lockWaitEntry;
		removedLockEntry[index] = new lockWaitEntry;
	}
	for(index = 0; index < MAX_CVS; index++){
		cvqueue[index] = new List;
		cvsDB[index].toBeDestroyedCount = 0;
		cvsDB[index].cvName = new char[MAX_CHARLEN];
		removedCVEntry[index] = new cvWaitEntry;
		CVEntry[index] = new cvWaitEntry;
	}
	for(index = 0; index < MAX_MVS; index++){
		mvsDB[index].toBeDestroyedCount = 0;
		mvsDB[index].mvName = new char[MAX_CHARLEN];
	}
	
	
	while(true) {
		
		// Refresh the variables in every iteration of the server
		type = 0;
		success = false;
		lockIndex = -1;		
		index = 0;
		waitLockIndex = -1;
		waitCVIndex = -1;
		signalLockIndex = -1;
		signalCVIndex = -1;
		broadcastBit = false;
		destroyLockIndex = -1;
	    destroyCVIndex = -1;
		name = NULL;
		MVIndex = 0;
		MVValue = 0;
		broadcastLockIndex = -1;
		broadcastCVIndex = -1;
		destroyMVIndex = -1;
		// REFRESH END
		
		
		// Wait for the reply message from the server nachos machine
		postOffice->Receive(SEND_MAILBOX, &inPktHdr, &inMailHdr, receiveMessage);
		printf("ServerHeart : Got \"%s\" from %d, box %d\n", receiveMessage, inPktHdr.from, inMailHdr.from);
		fflush(stdout);		
		
		rpcType = strtok(receiveMessage, " ");
		type = atoi(rpcType); 
		
		switch(type)
		{
			case 1:	//Create Lock
				name = strtok(NULL, " ");
				
				if(name == NULL){
					printf("ServerHeart : Lock Name not sent to the Server\n");
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;	
					outMailHdr.from = RECEIVE_MAILBOX;
					outPktHdr.from = serverMachineID;
					sprintf(sendMessage, "FAILURE : Lock Name is NULL");
					// Format the reply message to the client waiting for the Lock Index from the server
					outMailHdr.length = strlen(sendMessage) + 1;
					// Send the message
					success = postOffice->Send(outPktHdr, outMailHdr, sendMessage); 

					if ( !success ) {
						printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}	
					break;		
				}
				
				CreateLockRPC(name,  inPktHdr.from, inMailHdr.from);
				break;
				
			case 2:	//Acquire Lock
				acquireLockIndex = strtok(NULL, " ");
				lockIndex = atoi(acquireLockIndex);
				
				if(lockIndex == -1){
					printf("ServerHeart : LockIndex to acquire lock not passed to the Server\n");
					break;
				}
				
				AcquireLockRPC(lockIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 3:	//Release Lock	
				releaseLockIndex = strtok(NULL, " ");
				lockIndex = atoi(releaseLockIndex);
				if(lockIndex == -1){
					printf("ServerHeart : LockIndex to release lock not passed to the Server\n");
					break;;
				}
				
				ReleaseLockRPC(lockIndex, inPktHdr.from, inMailHdr.from);
				break;				
				
			case 4:	//Destroy Lock
				strDestroyLockIndex = strtok(NULL, " ");
				destroyLockIndex = atoi(strDestroyLockIndex);
				if(destroyLockIndex == -1){
					printf("ServerHeart : LockIndex to Destroy lock not passed to the Server\n");
					return;
				}
				DestroyLockRPC(destroyLockIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 5:	//Create CV
				name = strtok(NULL, " ");
				
				if(name == NULL){
					printf("ServerHeart : CV Name not sent to the Server\n");
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;	
					outMailHdr.from = RECEIVE_MAILBOX;
					outPktHdr.from = serverMachineID;
					sprintf(sendMessage, "FAILURE : CV Name is NULL");
					// Format the reply message to the client waiting for the Lock Index from the server
					outMailHdr.length = strlen(sendMessage) + 1;
					// Send the message
					success = postOffice->Send(outPktHdr, outMailHdr, sendMessage); 

					if ( !success ) {
						printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}	
					break;		
				}
				
				CreateCVRPC(name,  inPktHdr.from, inMailHdr.from);
				break;
			
			case 6:	//CV Wait
				strWaitCVIndex = strtok(NULL, " ");
				strWaitLockIndex = strtok(NULL, " ");
				waitCVIndex = atoi(strWaitCVIndex);
				waitLockIndex = atoi(strWaitLockIndex);
				
				if((waitLockIndex == -1)||(waitCVIndex == -1)){
					printf("ServerHeart : LockIndex or CVIndex for waitCV not passed to the Server\n");
					break;
				}
				CVWaitRPC(waitCVIndex, waitLockIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 7:	//CV Signal
				strSignalCVIndex = strtok(NULL, " ");
				strSignalLockIndex = strtok(NULL, " ");
				signalCVIndex = atoi(strSignalCVIndex);
				signalLockIndex = atoi(strSignalLockIndex);
				if((signalLockIndex == -1)||(signalCVIndex == -1)){
					printf("ServerHeart : LockIndex or CVIndex for SignalCV not passed to the Server\n");
					break;
				}
		
				CVSignalRPC(signalCVIndex, signalLockIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 8:	// Broadcast CV 
				strBroadcastCVIndex = strtok(NULL, " ");
				strBroadcastLockIndex = strtok(NULL, " ");
				broadcastCVIndex = atoi(strBroadcastCVIndex);
				broadcastLockIndex = atoi(strBroadcastLockIndex);
				if((broadcastCVIndex == -1)||(broadcastLockIndex == -1)){
					printf("ServerHeart : LockIndex or CVIndex for BroadcastCV not passed to the Server\n");
					break;
				}
				
				CVBroadcastRPC(broadcastCVIndex, broadcastLockIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 9:	//Destroy CV
				strDestroyCVIndex = strtok(NULL, " ");
				destroyCVIndex = atoi(strDestroyCVIndex);
				if(destroyCVIndex == -1){
					printf("ServerHeart : CV Index not sent to the Server\n");
					return;
				}
				
				CVDestroyRPC(destroyCVIndex, inPktHdr.from, inMailHdr.from);
				break;
			
			case 10: // Create MV
				name = strtok(NULL, " ");
				
				if(name == NULL){
					printf("ServerHeart : MV Name not sent to the Server\n");
					outPktHdr.to = inPktHdr.from;
					outMailHdr.to = inMailHdr.from;	
					outMailHdr.from = RECEIVE_MAILBOX;
					outPktHdr.from = serverMachineID;
					sprintf(sendMessage, "FAILURE : MV Name is NULL");
					// Format the reply message to the client waiting for the MV Index from the server
					outMailHdr.length = strlen(sendMessage) + 1;
					// Send the message
					success = postOffice->Send(outPktHdr, outMailHdr, sendMessage); 

					if ( !success ) {
						printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
						interrupt->Halt();
					}	
					break;		
				}
				
				CreateMVRPC(name,  inPktHdr.from, inMailHdr.from);
				break;
				
			case 11: // Get MV
				strMVIndex = strtok(NULL, " ");
				MVIndex = atoi(strMVIndex);
				
				if(MVIndex == -1){
					printf("ServerHeart : MV Index not passed to the Server\n");
					break;
				}
				
				GetMVRPC(MVIndex, inPktHdr.from, inMailHdr.from);
				break;
				
			case 12: // Set MV
				strMVIndex = strtok(NULL, " ");
				MVIndex = atoi(strMVIndex);
				strMVValue = strtok(NULL, " ");
				MVValue = atoi(strMVValue);
				
				if(MVIndex == -1){
					printf("ServerHeart : MV Index not passed to the Server\n");
					break;
				}
				
				SetMVRPC(MVIndex, MVValue, inPktHdr.from, inMailHdr.from);
				break;	
				
			case 13://Destroy MV
				strDestroyMVIndex = strtok(NULL, " ");
				destroyMVIndex = atoi(strDestroyMVIndex);
				if(destroyMVIndex == -1){
					printf("ServerHeart : CV Index not sent to the Server\n");
					return;
				}
				
				DestroyMVRPC(destroyMVIndex, inPktHdr.from, inMailHdr.from);
				break;	
		}		
	}
}
 
void
MailTest(int farAddr)
{
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}
