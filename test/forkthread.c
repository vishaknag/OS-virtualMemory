#include "syscall.h"
#define Dim 	20

int D[Dim][Dim];
int E[Dim][Dim];
int F[Dim][Dim];


int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

void matThread1()
{


    int i, j, k;
	
    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     A[i][j] = i;
	     B[i][j] = j;
	     C[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
			{
				C[i][j] += A[i][k] * B[k][j];
				/*  Print_1Arg("im here value = %d\n", C[i][j]); */
			}	
	Print_1Arg("Result ( Fork 1) = %d\n", C[Dim-1][Dim-1]);
    Exit(C[Dim-1][Dim-1]);		/* and then we're done */
}


void matThread2()
{

    int i, j, k;
	
    for (i = 0; i < Dim; i++)		/* first initialize the matrices */
	for (j = 0; j < Dim; j++) {
	     D[i][j] = i;
	     E[i][j] = j;
	     F[i][j] = 0;
	}

    for (i = 0; i < Dim; i++)		/* then multiply them together */
	for (j = 0; j < Dim; j++)
            for (k = 0; k < Dim; k++)
			{
				F[i][j] += D[i][k] * E[k][j];
				 
			}	
	Print_1Arg("Result ( Fork 2) = %d\n", F[Dim-1][Dim-1]);
    Exit(F[Dim-1][Dim-1]);		/* and then we're done */
}


void main()
{
	 Fork(matThread1); 

	Fork(matThread2);
}
