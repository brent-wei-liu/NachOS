// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    value --;
    //printf("Semaphore[%s] value[%d]\n",name,value);
    if(value < 0) { 			// semaphore not available
    	queue->Append((void *)currentThread);	// so go to sleep
    	currentThread->Sleep();
      //  printf("[%s] sleep at semaphore[%s] value[%d]\n",currentThread->getName(),name,value);
    } 
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    value ++;
    if(value <=0){
        thread = (Thread *)queue->Remove();
        if (thread != NULL) 	scheduler->ReadyToRun(thread);
    }
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName)
{
    name = debugName;
    thread = NULL; // No process holds this lock
    sleep = new Semaphore("sleep", 1 );  // Initialize semaphore to 0.
}

Lock::~Lock() 
{
//    delete mutex;
//    delete sleep;
}
void Lock::Acquire() 
{
    sleep->P();
    thread = currentThread;
}
void Lock::Release()
{
     ASSERT(isHeldByCurrentThread());
     thread = NULL;
     sleep->V();
}
bool Lock::isHeldByCurrentThread()
{
    if( thread == currentThread)    return true;
    else    return false;
}

//----------------------------------------------------------------------
// Condition::Condition
//----------------------------------------------------------------------

Condition::Condition(char* debugName)
{
    name = debugName;
    waitQueue = new List; // create a Semaphore queue
}

Condition::~Condition()
{
    delete waitQueue;
}

void Condition::Wait(Lock* conditionLock) 
{
    //printf("\nWait at CV[%s] currentThread:%s\n",name,currentThread->getName());
    Semaphore *waiter;
    ASSERT(conditionLock->isHeldByCurrentThread());
    waiter = new Semaphore("condition", 0);
    waitQueue->Append(waiter); // append a new waiter to the queue
    conditionLock->Release();
    waiter->P();
    conditionLock->Acquire();
    delete waiter;
}

void Condition::Signal(Lock* conditionLock)
{
    Semaphore *waiter;
    ASSERT(conditionLock->isHeldByCurrentThread());
    if (!waitQueue->IsEmpty())
    {
        waiter = (Semaphore *)waitQueue->Remove();
      //  printf("\nSignle at CV:%s\n",name);
	    waiter->V();
    }
}

void Condition::Broadcast(Lock* conditionLock) 
{
    while (!waitQueue->IsEmpty()) 
    {
        Signal(conditionLock);
    }
}


