// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"

extern "C" { int bzero(char *, int); };
extern int evictAPage(int pageToEvict);
Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    unsigned int i, size;
	int phyPageNo = 0;

	this->execute = executable;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);  
	
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                        // we need to increase the size
						// to leave room for the stack
   
	size = numPages * PageSize;
	spaceSize = size;
	
 //   ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
	
 
	ptoffset =  new pageTableWithOffset[numPages];
	ptOffsetLock->Acquire();
	
    for (i = 0; i < numPages; i++) 
	{
		// computing offset of all code and initialized data in address space
		if(i < (divRoundUp((noffH.code.size + noffH.initData.size), PageSize)))
		{	
			ptoffset[i].offset = (noffH.code.inFileAddr + (i * PageSize));
			ptoffset[i].whereisthepage = 0;
		}
		else
		{
			// computing offset of all uninitialized and stack in address space
			ptoffset[i].offset = (noffH.code.inFileAddr + (i * PageSize));
			ptoffset[i].whereisthepage = 2;
		}
    }
	ptOffsetLock->Release();	
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete ptoffset;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
	//	
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{	
	int dirtyIndex = 0;

		for( dirtyIndex = 0; dirtyIndex < 4; dirtyIndex++)
		{
			IntStatus oldLevel = interrupt->SetLevel(IntOff);
			if((machine->tlb[dirtyIndex].dirty))
			{	
					
				ipt[machine->tlb[dirtyIndex].physicalPage].dirty = true;
					
			}
			machine->tlb[dirtyIndex].valid = false;
			(void) interrupt->SetLevel(oldLevel);
		}	
}

//----------------------------------------------------------------------
// AddrSpace::GetNumPages
//  return the current number of pages in the Address space
//----------------------------------------------------------------------

int AddrSpace::GetNumPages()
{
	return numPages;
}

//----------------------------------------------------------------------
// AddrSpace::SetNumPages
//  Set the new current number of pages to the Address space
//----------------------------------------------------------------------

void AddrSpace::SetNumPages(unsigned int newNumPages)
{
	numPages = newNumPages;
}
 
//----------------------------------------------------------------------
// AddrSpace::GetPageTableRef
//  return the reference to the Page table of the current Address space
//----------------------------------------------------------------------

pageTableWithOffset* AddrSpace::GetPageTableRef()
{
	return ptoffset;
	
}
 
//----------------------------------------------------------------------
// AddrSpace::SetPageTableRef
//  return the reference to the Page table of the current Address space
//----------------------------------------------------------------------

void AddrSpace::SetPageTableRef(pageTableWithOffset *newPageTable)
{
	ptoffset = newPageTable;
} 




//---------------------------------------------------------------------
// AddrSpace::DeletePageTable
//  delete the pagetable associated with the address space
//---------------------------------------------------------------------

void AddrSpace::DeletePageTable() 
{
	delete ptoffset;
}


void AddrSpace :: updatePtOffset(int vpn)
{	
	ptoffset[vpn].whereisthepage = 1; 
	ptoffset[vpn].offset = position; 
} 

int AddrSpace::loadPageIntoIPT(int vpn)
{
	
	pageTableWithOffset* myptoffset;
	int pageToEvict;
	int pagelocation;
	int pageoffset;
	int locInMemory;
	
	myptoffset = currentThread->space->GetPageTableRef();
	pagelocation = myptoffset[vpn].whereisthepage;
	// checking if executable
	if(pagelocation == 0)
	{
		
		pageoffset =  myptoffset[vpn].offset;
		//ptOffsetLock->Release();
		bitMapObjectLock->Acquire();
		locInMemory = bitMapObject->Find();
		bitMapObjectLock->Release();
		if(fifoFlag)
		{
			
				if(locInMemory!=-1)
				{
					fifoLock->Acquire();
					fifo->Append((int *)locInMemory);
					fifoLock->Release();
				}
				else
				{
					// remove from the fifo queue
					fifoLock->Acquire();
					locInMemory=(int)fifo->Remove();
					fifo->Append((int *)locInMemory);
					evictAPage(locInMemory);
					fifoLock->Release();
				}
		}
		else if(randFlag == 2)
		{
			
			if(locInMemory == -1)
			{	// since no physical memory evicting it
					pageToEvict = rand()%NumPhysPages;
					locInMemory = evictAPage(pageToEvict);
			} 
		}
		bzero(&(machine->mainMemory[(locInMemory * PageSize)]), PageSize);
		execute->ReadAt(&(machine->mainMemory[(locInMemory * PageSize)]), PageSize, pageoffset);
		
	}	// checking if swapfile
	else if(pagelocation == 1)
	{
			pageoffset =  myptoffset[vpn].offset;		
			//ptOffsetLock->Release();
			bitMapObjectLock->Acquire();
			locInMemory = bitMapObject->Find();
			bitMapObjectLock->Release();
			
			if(fifoFlag)
			{
				
				if(locInMemory!=-1)
				{
					fifoLock->Acquire();
					fifo->Append((int *)locInMemory);
					fifoLock->Release();
				}
				else
				{
					// remove from the fifo queue
					fifoLock->Acquire();
					locInMemory=(int)fifo->Remove();
					fifo->Append((int *)locInMemory);
					evictAPage(locInMemory);
					fifoLock->Release();
				}
		}
		else if(randFlag == 2)
		{
			
			if(locInMemory == -1)
			{	// since no physical memory evicting it
					pageToEvict = rand()%NumPhysPages;
					locInMemory = evictAPage(pageToEvict);
			} 
		}
			bzero(&(machine->mainMemory[(locInMemory * PageSize)]), PageSize);
			swapFile->ReadAt(&(machine->mainMemory[(locInMemory * PageSize)]), PageSize, pageoffset);
			
			//swapFileLock->Release();
	}
	else if( pagelocation == 2) // checking if neither
	{
		
		//ptOffsetLock->Release();
		bitMapObjectLock->Acquire();
		locInMemory = bitMapObject->Find();
		bitMapObjectLock->Release();
		
		if(fifoFlag)
		{
			
				if(locInMemory!=-1)
				{
					fifoLock->Acquire();
					fifo->Append((int *)locInMemory);
					fifoLock->Release();
				}
				else
				{
					// remove from the fifo queue
					fifoLock->Acquire();
					locInMemory=(int)fifo->Remove();
					fifo->Append((int *)locInMemory);
					evictAPage(locInMemory);
					fifoLock->Release();
				}
		}
		else if(randFlag == 2)
		{
		
			if(locInMemory == -1)
			{	// since no physical memory evicting it
					pageToEvict = rand()%NumPhysPages;
					locInMemory = evictAPage(pageToEvict);
			} 
		}
		bzero(&(machine->mainMemory[(locInMemory * PageSize)]), PageSize);
		
		   
	}
		// updating the IPT 
		ipt[locInMemory].virtualPage = vpn;
		ipt[locInMemory].physicalPage = locInMemory;
		ipt[locInMemory].valid = TRUE;
		ipt[locInMemory].use = FALSE;
		ipt[locInMemory].dirty = FALSE;
		ipt[locInMemory].processID = currentThread->pid;
		
	return locInMemory;
}





