/* syscalls.h 
 * 	Nachos system call interface.  These are Nachos kernel operations
 * 	that can be invoked from user programs, by trapping to the kernel
 *	via the "syscall" instruction.
 *
 *	This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define SC_Halt			0
#define SC_Exit			1
#define SC_Exec			2
#define SC_Join			3
#define SC_Create		4
#define SC_Open			5
#define SC_Read			6
#define SC_Write		7
#define SC_Close		8
#define SC_Fork			9
#define SC_Yield		10
#define SC_CreateLock	11
#define SC_Acquire		12
#define SC_Release		13
#define SC_DestroyLock	14
#define SC_CreateCV		15
#define SC_Wait			16
#define SC_Signal		17
#define SC_Broadcast	18
#define SC_DestroyCV	19
#define SC_Scan			20
#define SC_Print_Stmt	21
#define SC_Print_1Arg	22
#define SC_Print_2Arg	23
#define SC_Print_3Arg	24
#define SC_Sprint		25
#define SC_Print_Str	26
#define SC_CreateMV		27
#define SC_GetMV		28
#define SC_SetMV		29
#define SC_MVDestroy	30
	
#define MAXFILENAME 256

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void Halt();		
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);	

/* A unique identifier for an executing user program (address space) */
typedef int SpaceId;	
 
/* Run the executable, stored in the Nachos file "name", and return the 
 * address space identifier
 */
SpaceId Exec(char* name, int len);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id); 	
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;	

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with "name" */
void Create(char *name, int size);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId Open(char *name, int size);

/* Write "size" bytes from "buffer" to the open file. */
void Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

/* Close the file, we're done reading and writing to it. */
void Close(OpenFileId id);



/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork(void (*func)());

/* Exit Syscall */
void Exit(int status);

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield();		

/* Create a Lock in the same address space as the current Thread */
int CreateLock(char* name, int len);

/* Acquire the Lock which the current Thread created in its own address space */
int Acquire(unsigned int lockIndex);

/* Release the Lock which the current Thread acquired */
int Release(unsigned int lockIndex);

/* Destroy the Lock which the current Thread created in its own address space */
int DestroyLock(unsigned int lockIndex);

/* Create a CV in the same address space as the current Thread */
int CreateCV(char* name, int len);

/* Syscall to make the current thread go on wait on the condition variable indexed by "CVIndex" */
int Wait(unsigned int CVIndex, unsigned int lockIndex);

/*  Syscall to make the current thread signal the thread waiting on the condition 
 *	variable indexed by "CVIndex" 
 */
int Signal(unsigned int CVIndex, unsigned int lockIndex);

/*  Syscall to make the current thread signal all the threads waiting on the condition 
 *	variable indexed by "CVIndex" 
 */ 
int Broadcast(unsigned int CVIndex, unsigned int lockIndex);

/* Destroy the CV which the current Thread created in its own address space */
int DestroyCV(unsigned int CVIndex);

/* Scan Syscall */
int Scan(char* name);

/* Print syscall which prints only a statement, it has no access specifiers and values */
void Print_Stmt(char* name);

/* Print syscall which prints a statement with a single access specifier */
void Print_1Arg(char* name, int arg1);

/* Print syscall which prints a statement with two access specifiers */
void Print_2Arg(char* name, int arg1, int arg2);

/* Print syscall which prints a statement with three access specifiers */
void Print_3Arg(char* name, int arg1, int arg2, int arg3);

/* Print a String onto the standard output */
void Print_Str(char* Stmt, char* str);

/* Sprint syscall which concatenates a string with integer
	This is used to form the thread name with the Id appended to it */
char* Sprint(char* dest, char* accessSpec, char* src, int id);	

/* Create a Monitor variable on the server*/
int CreateMV(char* name, int len);

/* Get the value of the Monitor variable*/
int GetMV(int MVIndex);

/* Set the value of the Monitor variable*/
int SetMV(int MVIndex, int MVValue);

/* Destroy the MV on the server */
int DestroyMV(int MVIndex);

#endif /* IN_ASM */

#endif /* SYSCALL_H */
