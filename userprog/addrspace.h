// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256


class pageTableWithOffset 
{
	public: int whereisthepage; 
			// 0 ---> In Executable
			// 1 ---> In Swap File
			// 2 ---> Neither ( case like uninitialized data and stack )
			
			int offset; 
			// store the offset of each page 
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space
 
    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
	int GetNumPages();			// get the size of the address space
	void SetNumPages(unsigned int newNumPages);	// set the new page count 
	//TranslationEntry* GetPageTableRef();	// get the pagetable reference of the current address space
	pageTableWithOffset* GetPageTableRef();
	void SetPageTableRef(pageTableWithOffset *newPageTable);	// set the new pagetable to the address space
	void DeletePageTable();		// delete the old pageTable	
    Table fileTable;			// Table of openfiles
	unsigned int spaceSize;		// size of the address space
	
	//OpenFile* GetFileRef();		// return the reference for the executable
	//void SetFileRef(OpenFile* fileRef);		// set the new file name
	int loadPageIntoIPT(int vpn);
	void updatePtOffset(int vpn);	

 private:
   // TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    pageTableWithOffset *ptoffset;
	unsigned int numPages;		// Number of pages in the virtual 
					// address space
	OpenFile *execute; // pointer which stores the executable location
};

#endif // ADDRSPACE_H
