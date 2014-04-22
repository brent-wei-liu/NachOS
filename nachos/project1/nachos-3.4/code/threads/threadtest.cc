// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
// testnum is set in main.cc
int testnum = 1;


#if defined(CHANGED) && defined(THREADS)


int SharedVariable = 0; 
Semaphore *sem,*barrierSem;
Lock *lock,*barrierLock;
int barrier = 0; 
void SimpleThread(int which)
{
#if defined(HW1_SEMAPHORES)
    int num, val, b; 
    for(num = 0; num < 5; num++) { 
        sem->P();

        val = SharedVariable; 
        printf("*** thread %d sees value %d\n", which, val); 
        SharedVariable = val+1;

        sem->V();

    
        //printf("Thread %d set value to %d\n", which, val+1); 
        currentThread->Yield(); 
    } 

    barrierSem->P();
    barrier --;
    barrierSem->V();
    
    while(true){
        barrierSem->P();
        b = barrier;
        barrierSem->V();
        if(b<=0) break;
   //     printf("b:%d\n",b);
        currentThread->Yield();
    }
    
    
    sem->P();          
    val = SharedVariable;
    sem->V();
    printf("Thread %d sees final value %d\n", which, val);

#endif

#if defined(HW1_LOCKS)
    int num, val,b; 
    for(num = 0; num < 5; num++) { 

        lock->Acquire();
       
        val = SharedVariable; 
        printf("*** thread %d sees value %d\n", which, val); 
        currentThread->Yield(); 
        SharedVariable = val+1;

        lock->Release();
    
        //printf("Thread %d set value to %d\n", which, val+1); 
        currentThread->Yield(); 
    } 
    barrierLock->Acquire();
    barrier --;
    barrierLock->Release();
    
    while(true){
        barrierLock->Acquire();
        b = barrier;
        barrierLock->Release();
        if(b<=0) break;
   //     printf("b:%d\n",b);
        currentThread->Yield();
    }
 
    lock->Acquire();
    val = SharedVariable;
    lock->Release();
    printf("Thread %d sees final value %d\n", which, val); 
#endif
}

void ThreadTest(int n)
{
    DEBUG('t', "Entering ThreadTest");
    sem = new Semaphore("Semaphore",1);
    barrierSem = new Semaphore("Barrier Semaphore",1);
    barrier = n;
    
    lock = new Lock("Lock");
    barrierLock = new Lock("barrier Lock");
    for(int i=0;i<n;i++){
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThread, i);
    }
}


#else
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }

}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 2);
    SimpleThread(0);
}


//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}
#endif
