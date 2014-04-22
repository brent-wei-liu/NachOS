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

int
main()
{
    char * filename = "write.txt";
    int i,fd;
    int count;
    char buf[10];
    fd = Open(filename);
    if(fd == -1){
        Print("Can not open file!\n");
        Exit(-1);
    }else{
        Print("File Opened!\n");
    }
    
    Print("Write File:");
    Write("Hello world!", 5, fd); 

    Close(fd);

    Halt();
}
