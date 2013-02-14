// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "synch.h"
//added in part 3 project 3
#include "translate.h"
#include "ctime"
//end
#define MAX_LOCKS 500
#define MAX_CVS 500
#define MAX_MVS 500
#define MAX_PROCESSES 50
#define MAX_THREADS 500
#define MAX_CHARLEN 50

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
extern Lock* kernelLockListLock;	// Kernel lock for user program Lock list
extern Lock* kernelCVListLock;		// Kernel lock for user program CV list 
extern Lock* processTableLock;		// Kernel lock for process table 
extern Lock *pageTableLock[MAX_PROCESSES];	// Kernel Locks for pagetables of all the processes
extern Lock *activeProcessesLock;		// shared by exec and exit syscalls
extern Lock *IPTLock;
extern Lock *ptOffsetLock;
// List of Locks Maintained by the Kernel
typedef struct
{
	Lock *lock;
	AddrSpace *lockAddrSpace;
	int usageCounter;
	bool toBeDestroyed;
}KernelLock;
extern KernelLock lockList[MAX_LOCKS];	// List of user locks maintained by kernel

// List of CVs Maintained by the Kernel
typedef struct
{
	Condition *condition;
	AddrSpace *conditionAddrSpace;
	int usageCounter;
	bool toBeDestroyed;
}KernelCV;
extern KernelCV CVList[MAX_CVS];	// List of user CVs maintained by kernel

// Structure representing the process table 
typedef struct
{
	int Pid;
	int threadCount;
	int activeThreadCount;
	AddrSpace *addrSpace;
	int stackTops[MAX_THREADS];
	OpenFile *executable;
}Process;
extern Process processTable[MAX_PROCESSES];
extern int currentTLB;
extern unsigned int nextLockIndex ;
extern unsigned int nextCVIndex;
extern int processCount;
extern int noOfActiveProcesses;
extern int position; // position in swap file
class IPTEntry : public TranslationEntry
{
	public:
			int processID;
			
			IPTEntry();
			~IPTEntry();
		
		 
};

extern IPTEntry *ipt;
extern OpenFile *swapFile;
extern Lock *swapFileLock;
extern AddrSpace *mainProcess;
//extern pageTableWithOffset *ptoffset; 
//extern int SWAPFILESIZE;
extern Lock *fifoLock;
extern List *fifo;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;

#ifdef CHANGED
extern int clientMachineID;
extern int serverMachineID;
extern int RECEIVE_MAILBOX;
extern int SEND_MAILBOX;

extern int createLockType;
extern int acquireLockType;
extern int releaseLockType;
extern int destroyLockType;
extern int createCVType;
extern int waitCVType;
extern int signalCVType;
extern int broadcastCVType;
extern int destroyCVType;
extern int createMVType;
extern int getMVType;
extern int setMVType;
extern int destroyMVType;
#endif

#endif

#ifdef CHANGED
// creating a bitmap object 
extern BitMap* bitMapObject; 
extern Lock* bitMapObjectLock;
extern int randFlag;
extern int fifoFlag;

#endif 

#endif // SYSTEM_H
