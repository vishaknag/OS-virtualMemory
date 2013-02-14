// system.cc 
//	Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
//int SWAPFILESIZE = 32000;
// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;			// the thread we are running now
Thread *threadToBeDestroyed;  		// the thread that just finished
Scheduler *scheduler;			// the ready list
Interrupt *interrupt;			// interrupt status
Statistics *stats;			// performance metrics
Timer *timer;				// the hardware timer device,
					// for invoking context switches
#ifdef CHANGED

//creating a object for bitmap class
BitMap* bitMapObject;
Lock* bitMapObjectLock; 
#endif 

#ifdef FILESYS_NEEDED
FileSystem  *fileSystem;
#endif

#ifdef FILESYS
SynchDisk   *synchDisk;
#endif

#ifdef USER_PROGRAM	// requires either FILESYS or FILESYS_STUB
Machine *machine;	// user program memory and registers

KernelLock lockList[MAX_LOCKS];

KernelCV CVList[MAX_CVS];

Process processTable[MAX_PROCESSES];

// Index of the next Lock to be created in the Kernel Lock List
unsigned int nextLockIndex = 0;

// Index of the next CV to be created in the Kernel CV List
unsigned int nextCVIndex = 0;

// current index to the TLB
int currentTLB = 0;

// Count for the number of processes created till now
// Value can be accesses only on acquiring the Lock processtableLock
// processCount = 0 indicates there is one process
// This value is never decremented
int processCount = -1;

// to keep track of the current number of active processes in the system
int noOfActiveProcesses = 0;

// Lock to establish mutual exclusion on the Lock List
Lock *kernelLockListLock = NULL;

// Lock to establish mutual exclusion on the CV List
Lock *kernelCVListLock = NULL;

// Lock to establish mutual exclusion on the process table
Lock *processTableLock = NULL;
Lock *pageTableLock[MAX_PROCESSES];

Lock *activeProcessesLock = NULL;

// object of IPTEntry
IPTEntry *ipt;

// Lock to establish mutual exclusion on the updatetlb
Lock *IPTLock =  NULL;

// object of page table which contains offset
//pageTableWithOffset *ptoffset = NULL;
OpenFile *swapFile = NULL;
Lock *swapFileLock = NULL;
int position = 0; // position in swap file
Lock *ptOffsetLock = NULL;
AddrSpace *mainProcess = NULL;
Lock *fifoLock  = NULL;
List *queue = NULL;
List *fifo;
int randFlag = 0;
int fifoFlag = 0;

#endif

#ifdef NETWORK
#ifdef CHANGED

PostOffice *postOffice;
int RECEIVE_MAILBOX = 1;
int SEND_MAILBOX = 0;
int createLockType = 1;
int acquireLockType = 2;
int releaseLockType = 3;
int destroyLockType = 4;
int createCVType = 5;
int waitCVType = 6;
int signalCVType = 7;
int broadcastCVType = 8;
int destroyCVType = 9;
int createMVType = 10;
int getMVType = 11;
int setMVType = 12;
int destroyMVType = 13;

// Machine ID of the client
int clientMachineID = -1;
int serverMachineID = 0;
int serverMailBoxNumber = 0;

#endif
#endif


// External definition, to allow us to take a pointer to this function
extern void Cleanup();

// definition for IPTEntry Class functions

IPTEntry :: IPTEntry()
{
	
	processID = 0;
	
	
}

IPTEntry :: ~IPTEntry()
{

}
	

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	Interrupt handler for the timer device.  The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
static void
TimerInterruptHandler(int dummy)
{
    if (interrupt->getStatus() != IdleMode)
	interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
// 	Initialize Nachos global data structures.  Interpret command
//	line arguments in order to determine flags for the initialization.  
// 
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void
Initialize(int argc, char **argv)
{
    int argCount;
    char* debugArgs = "";
    bool randomYield = FALSE;

	fileSystem->Create("swapfile", 3200) ;
	swapFile = fileSystem->Open("swapfile");
	if (swapFile == NULL)
	{
		printf("Exec_Syscall : Unable to open file \n");
		return;
   	}
#ifdef CHANGED
// making the bitmap have same number of entries in physical memory 
bitMapObjectLock = new Lock("BitMapLock");
bitMapObject = new BitMap(NumPhysPages); 
#endif 
	
#ifdef USER_PROGRAM
    bool debugUserProg = FALSE;	// single step user program
	kernelLockListLock = new Lock("kernelLockListLock");
	kernelCVListLock = new Lock("kernelCVListLock");
	processTableLock = new Lock("processTableLock");
	activeProcessesLock = new Lock("activeProcessesLock");
	IPTLock = new Lock("IPTLock"); 
	swapFileLock = new Lock("swapFileLock");
	ptOffsetLock = new Lock("ptOffsetLock");
	for( int index = 0; index < NumPhysPages; index++)
	ipt= new IPTEntry[index];
	for(int i = 0; i < MAX_PROCESSES; i++){
		pageTableLock[i] = new Lock("pageTableLock");
	}
	fifoLock = new Lock("fifoLock");
	fifo = new List;
#endif
#ifdef FILESYS_NEEDED
    bool format = FALSE;	// format disk
#endif
#ifdef NETWORK
    double rely = 1;		// network reliability
    int netname = 0;		// UNIX socket name
#endif
    
    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
	if (!strcmp(*argv, "-d")) {
	    if (argc == 1)
		debugArgs = "+";	// turn on all debug flags
	    else {
	    	debugArgs = *(argv + 1);
	    	argCount = 2;
	    }
	} else if (!strcmp(*argv, "-rs")) {
	    ASSERT(argc > 1);
	    RandomInit(atoi(*(argv + 1)));	// initialize pseudo-random
						// number generator
	    randomYield = TRUE;
	    argCount = 2;
	}
#ifdef USER_PROGRAM
	if (!strcmp(*argv, "-s"))
	    debugUserProg = TRUE;
#endif
#ifdef FILESYS_NEEDED
	if (!strcmp(*argv, "-f"))
	    format = TRUE;
#endif
#ifdef NETWORK
	if (!strcmp(*argv, "-l")) {
	    ASSERT(argc > 1);
	    rely = atof(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-m")) {
	    ASSERT(argc > 1);
	    netname = atoi(*(argv + 1));
	    argCount = 2;
	}
#endif
    }

    DebugInit(debugArgs);			// initialize DEBUG messages
    stats = new Statistics();			// collect statistics
    interrupt = new Interrupt;			// start up interrupt handling
    scheduler = new Scheduler();		// initialize the ready queue
    if (randomYield)				// start the timer (if needed)
	timer = new Timer(TimerInterruptHandler, 0, randomYield);

    threadToBeDestroyed = NULL;

    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state. 
    currentThread = new Thread("main");		
    currentThread->setStatus(RUNNING);

    interrupt->Enable();
    CallOnUserAbort(Cleanup);			// if user hits ctl-C
    
#ifdef USER_PROGRAM
    machine = new Machine(debugUserProg);	// this must come first
#endif

#ifdef FILESYS
    synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
    fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
    postOffice = new PostOffice(netname, rely, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void
Cleanup()
{
    printf("\nCleaning up...\n");
#ifdef NETWORK
    delete postOffice;
#endif
    
#ifdef USER_PROGRAM
    delete machine;
#endif

#ifdef FILESYS_NEEDED
    delete fileSystem;
#endif

#ifdef FILESYS
    delete synchDisk;
#endif
    
    delete timer;
    delete scheduler;
    delete interrupt;
    
    Exit(0);
}


