/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

void func()
{
    int a;

    for(a=0; a<=3; a++){
        Print("I am the child\n");
    }
    Exit(2);
}
int
main()
{
    int i,pid;
    pid = Fork(func);
    
    
    Print("I am the Parent!\n");
    Yield();
    for(i=0; i<2; i++){
        Print("I am the Parent!\n");
    }
}
