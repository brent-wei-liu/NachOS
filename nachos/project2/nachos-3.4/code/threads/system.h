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
#include "bitmap.h"
extern BitMap* bitmap; 
struct ProcEntry {
   ProcessControlBlock * pcb;
};

extern int * procNumber;
class ProcessTable {
    public:
        ProcessTable(int procEntries) {
            procTable = new ProcEntry[procEntries];
            for (int i=0; i<procEntries; i++) {
                procTable[i].pcb=NULL;
            }
            size = procEntries;
        }
        int addProcess(Thread * t) {  // assign pid for given thread
            int i=0;
            while (i<size) {
                if (procTable[i].pcb == NULL) {
                    procTable[i].pcb = t->space->getPCB();
                    t->space->getPCB()->parentBlockSem = new Semaphore(
                                                                "blocking",0);
                    return i;
                }
                i++;
            }
            return -1;
        }
        
        ProcessControlBlock * returnProcess(int pid) { //checks if given pid exists
            if(pid > size){
                printf("returnProcess error , pid:%d\n",pid);
                return NULL;
            }
            if (procTable[pid].pcb!=NULL){  //not-empty
                return procTable[pid].pcb;
            }
            else 
                return NULL;
        }
        
        bool removeProcess(int pid) { //removes given pid from list;
            if (procTable[pid].pcb!=NULL){
                procTable[pid].pcb= NULL;
                return true;                //successful removal
            }
            else 
                return false;               //failure 
        }
        int nextPid(){
            int i=0;
            while (i<size) {
                if (procTable[i].pcb == NULL) return i;
                i++;
            }
            return -1;
        }
    private:    
        ProcEntry * procTable;
        int size;
};
extern ProcessTable * procTable;


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
#endif

#endif // SYSTEM_H
