# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles CarlsJr proj3testcases forkthread forkthreadSort rpctest

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

CarlsJr.o: CarlsJr.c
	$(CC) $(CFLAGS) -c CarlsJr.c
CarlsJr: CarlsJr.o start.o
	$(LD) $(LDFLAGS) start.o CarlsJr.o -o CarlsJr.coff
	../bin/coff2noff CarlsJr.coff CarlsJr

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles
	
proj3testcases.o: proj3testcases.c
	$(CC) $(CFLAGS) -c proj3testcases.c
proj3testcases: proj3testcases.o start.o
	$(LD) $(LDFLAGS) start.o proj3testcases.o -o proj3testcases.coff
	../bin/coff2noff proj3testcases.coff proj3testcases
	
forkthread.o: forkthread.c
	$(CC) $(CFLAGS) -c forkthread.c
forkthread: forkthread.o start.o
	$(LD) $(LDFLAGS) start.o forkthread.o -o forkthread.coff
	../bin/coff2noff forkthread.coff forkthread
	
forkthreadSort.o: forkthreadSort.c
	$(CC) $(CFLAGS) -c forkthreadSort.c
forkthreadSort: forkthreadSort.o start.o
	$(LD) $(LDFLAGS) start.o forkthreadSort.o -o forkthreadSort.coff
	../bin/coff2noff forkthreadSort.coff forkthreadSort
	
rpctest.o: rpctest.c
	$(CC) $(CFLAGS) -c rpctest.c
rpctest: rpctest.o start.o
	$(LD) $(LDFLAGS) start.o rpctest.o -o rpctest.coff
	../bin/coff2noff rpctest.coff rpctest
	
clean:
	rm -f *.o *.coff
