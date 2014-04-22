// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

void Join_SystemCall(int pid);
void Print_SystemCall();
void Exit_SystemCall(int status, SpaceId pid); 
void Kill_SystemCall(SpaceId killerPid, SpaceId pidKilled); 
void Fork_SystemCall(int pid);
void Exec_SystemCall();
void CleanupAndExit(); 
void increaseProgramCounter();   //instruction program counter is increased
void CopyStringToKernel(int FromUserAddress, char *ToKernelAddress);

    // this may happen either because of an exception 
                            // or because the process voluntarily exits

void updatePC(){

        // Note that we have to maintain three PC registers, 
        // namely : PCReg, NextPCReg, PrevPCReg. 
        // (See machine/machine.cc, machine/machine.h) for more details.
        int pc, nextpc, prevpc;

        // Read PCs
        prevpc = machine->ReadRegister(PrevPCReg);
        pc = machine->ReadRegister(PCReg);
        nextpc = machine->ReadRegister(NextPCReg);

        // Update PCs
        prevpc = pc;
        pc = nextpc;
        nextpc = nextpc + 4;    // PC incremented by 4 in MIPS

        // Write back PCs
        machine->WriteRegister(PrevPCReg, prevpc);
        machine->WriteRegister(PCReg, pc);
        machine->WriteRegister(NextPCReg, nextpc);
}

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		vaddr -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

#define BUF_SIZE 1024
char buf[BUF_SIZE];

void threadFunc(int which)
{

    for(;;){
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {     // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
        machine->Run();
    }
    
}

void threadExecFunc(int value){
    Thread * thread = (Thread *) value; 
 
    currentThread->space->InitRegisters();
    //for(;;){
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());
    //machine->DumpState();
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {     // if there is an address space
    //    currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
    machine->Run();
    //}
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int vaddr = machine->ReadRegister(4);
    SpaceId pid = currentThread->space->getPCB()->getPID();
    if ((which == SyscallException) && (type == SC_Halt)) {
	    DEBUG('a', "Shutdown, initiated by user program.\n");
   	    interrupt->Halt();
        
    }else if((which == SyscallException) && (type == SC_Exit)){
        Exit_SystemCall(vaddr, pid);
        updatePC();
        
    }else if((which == SyscallException) && (type == SC_Exec)){
         printf("System Call: [%d] invoked Exec.\n",pid);
	     Exec_SystemCall();
        
    }else if((which == SyscallException) && (type == SC_Join)){
        printf("System Call: [%d] invoked Join.\n",pid);
        //printf("System Call: [%d] invoked Join(%d)\n",pid,vaddr);
        Join_SystemCall(vaddr);
    /*
    }else if((which == SyscallException) && (type == SC_Create)){
        DEBUG('a',"SC_Create, initiate by user program.\n");
        interrupt->Create();
    }else if((which == SyscallException) && (type == SC_Open)){
        DEBUG('a',"SC_Open, initiate by user program.\n");
        interrupt->Open();
    }else if((which == SyscallException) && (type == SC_Read)){
        DEBUG('a',"SC_Read, initiate by user program.\n");
        interrupt->Read();
    }else if((which == SyscallException) && (type == SC_Write)){
        DEBUG('a',"SC_Write, initiate by user program.\n");
        interrupt->SC_Write();
    }else if((which == SyscallException) && (type == SC_Close)){
        DEBUG('a',"SC_Close, initiate by user program.\n");
        interrupt->Close();
        */
    }else if((which == SyscallException) && (type == SC_Fork)){
        printf("System Call: [%d] invoked Fork\n",pid);
        Fork_SystemCall(pid);
    }else if((which == SyscallException) && (type == SC_Yield)){
        printf("System Call: [%d] invoked Yield\n",pid);
        //printf("System Call: %d invoked Yield\n", SpaceId);
        updatePC();
        currentThread->SaveUserState(); 
        currentThread->space->SaveState();
        currentThread->Yield();
        
    }else if((which == SyscallException) && (type == SC_Kill)){
        printf("System Call: [%d] invoked Kill\n",pid);
        Kill_SystemCall(pid, vaddr);
        updatePC();
        
    }else if((which == SyscallException) && (type == SC_Print)){
        Print_SystemCall();
        
    }else {
	    printf("Unexpected user mode exception %d %d\n", which, type);
	    ASSERT(FALSE);
    }
}

void Fork_SystemCall(int currentPid){
    SpaceId pid;
    int new_pc , new_nextpc, new_prevpc, pc, nextpc, prevpc;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    new_pc = machine->ReadRegister(4);
    // Read PCs
    prevpc = machine->ReadRegister(PrevPCReg);
    pc = machine->ReadRegister(PCReg);
    nextpc = machine->ReadRegister(NextPCReg);

    AddrSpace *space = currentThread->space->Clone();
    if(space == NULL){
    printf("Not Enough Memory for Child Process %d\n",procTable->nextPid());       
    }else{
printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n",
            currentPid,
            new_pc,
            space->numPages);

       
    new_prevpc = pc;
    new_pc = new_pc;
    new_nextpc = new_pc + 4;    // PC incremented by 4 in MIPS

    // Write back PCs
    machine->WriteRegister(PrevPCReg, new_prevpc);
    machine->WriteRegister(PCReg, new_pc);
    machine->WriteRegister(NextPCReg, new_nextpc);

    Thread *t = new Thread("forked thread");
    t->space = space;
    t->SaveUserState();
    pid = procTable->addProcess(t);
    t->space->getPCB()->setPID(pid);
    t->space->getPCB()->setParentPID(currentPid);
    DEBUG('t', "PID (%d) assigned for new thread\n", pid);
    machine->WriteRegister(2, pid);
    t->space->getPCB()->setParent(currentThread);
    t->space->getPCB()->setThreadPointer(t);
    //DEBUG('t', "Just before thread execution starts\n");
    (*procNumber)++;
    t->Fork(threadFunc, (int)t);
    }
    
    machine->WriteRegister(PrevPCReg,prevpc);
    machine->WriteRegister(PCReg,pc);
    machine->WriteRegister(NextPCReg,nextpc);
    updatePC();
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Exec_SystemCall(){
    int add;
    int sizeadd; 
    char * filename;
    OpenFile * executable;
    AddrSpace * space;
    Thread * thread;
    SpaceId pid;
    SpaceId currentPid = currentThread->space->getPCB()->getPID();
     
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    add= machine->ReadRegister(4); // 1st parameter, file name
    sizeadd = machine->ReadRegister(5); // the 2nd parameter, file size
    
    filename = new char[sizeadd];
    CopyStringToKernel(add,  filename);
    
    executable = fileSystem->Open(filename);
    
    if (executable == NULL) {
    	printf("Error, unable to open file: [%s]\n", filename); 
    	CleanupAndExit();
        machine->WriteRegister(2, -1);
    	return;
    }
    printf("Exec Program: %d loading [\"%s\"]\n", currentPid,filename);


    space = new AddrSpace(executable);
    if(space->numPages==0){
    printf("Not Enough Memory for Child Process %d\n",procTable->nextPid());
    delete space;
    }else{
        thread = new Thread("child thread");
        thread->space = space;
        
        pid = procTable->addProcess(thread);
        thread->space->getPCB()->setPID(pid);
        thread->space->getPCB()->setParentPID(currentPid);
        machine->WriteRegister(2, pid);
        
        thread->space->getPCB()->setParent(currentThread);
        (*procNumber)++;
        thread->Fork(threadExecFunc, (int) thread);
    }
    updatePC();
    delete filename;
    delete executable;			// close file
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Print_SystemCall() {
    //DEBUG('t',"SC_Print, initiate by user program.\n");
    // Get the string from user space.
    int vaddr = machine->ReadRegister(4);
    int size = 0;
    buf[BUF_SIZE - 1] = '\0';               // For safety.
    do{
        // Invoke ReadMem to read the contents from user space
        machine->ReadMem(vaddr,    // Location to be read
            sizeof(char),      // Size of data to be read
            (int*)(buf+size)   // where the read contents 
            );                 // are stored
        // Compute next address
        vaddr+=sizeof(char);    size++;
    } while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

    size--;
    DEBUG('a', "Size of string = %d", size);

    printf("%s", buf);
    bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
    updatePC();

}

void Join_SystemCall(int pid) {
    if (procTable->returnProcess(pid) != NULL) { 
        //parent for pid should be current thread     
        ASSERT(procTable->returnProcess(pid)->getParent() == currentThread);  
        DEBUG('t', "Just before setting semaphore in blocked mode\n");
        procTable->returnProcess(pid)->setJoinedID(currentThread);
        procTable->returnProcess(pid)->parentBlockSem->P();
        machine->WriteRegister(2, currentThread->space->getPCB()->returnExit ); //HALM need to return the value from the exit system call
    } else {
        machine->WriteRegister(2, -1);
    }
    updatePC();
}

void Exit_SystemCall(int status, SpaceId pid) {
    printf("System Call: [%d] invoked Exit\n", pid);
    SpaceId parentPid = currentThread->space->getPCB()->getParentPID(); 
//    printf("debug:procTable:%d ppid:%d\n",procTable,parentPid);
    if(procTable->returnProcess(parentPid) != NULL ){
       if(currentThread->space->getPCB()->getParent() == currentThread->space->getPCB()->getJoinedID() ){
         procTable->returnProcess( currentThread->space->getPCB()->getParentPID() )->returnExit = status;
         currentThread->space->getPCB()->parentBlockSem->V();    
      }
    }else{
      printf("My parent is dead (Exit)\n");   
    }
    procTable->removeProcess(pid);
    procNumber = procNumber - 1;
    printf("Process [%d] exits with [%d]\n", pid, status);

    currentThread->Finish();
    
}

void Kill_SystemCall(SpaceId killerPid, SpaceId pidKilled) {
   printf("System Call: %d invoked Kill\n", killerPid);
   ProcessControlBlock *pcb = procTable->returnProcess(pidKilled);
   Thread *t = pcb->getThreadPointer();
   if(pcb != NULL){
      t->killFlag = true;
      
      procTable->removeProcess(pidKilled);
      procNumber = procNumber - 1;
      printf("Process [%d] killed process [%d]\n", killerPid, pidKilled);
      machine->WriteRegister(2, 0);
   } else { 
      printf("Process %d cannot kill process %d: doesn't exist\n", killerPid, pidKilled);
      machine->WriteRegister(2, -1);
   }
}

void CleanupAndExit() {
    SpaceId pid;
    ProcessControlBlock *pcb = currentThread->space->getPCB();
    pid = pcb->getPID();
    
//    DEBUG('x', "Cleaning up file table for process %s\n", currentThread->getName());
//    currentThread->space->getPCB()->cleanupFileEntries();
    
//    currentThread->space->freeFrames();
//    DEBUG('x', "Finished cleaning up allocated frames for process %s\n", currentThread->getName());
    
    //DEBUG('t', "Just before unblocking, Semaphore (0x%x)\n", currentThread->space->getPCB()->parentBlockSem);
    currentThread->space->getPCB()->parentBlockSem->V();    
    procTable->removeProcess(pid);
    if (--(*procNumber))   
        currentThread->Finish();
    else
        interrupt->Halt();   //no other processes left 
}

void increaseProgramCounter() {
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg)); // Previous program counter (for debugging)
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg) );  //Current program counter
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) +4); //Next program counter (for branch delay)
}

void CopyStringToKernel(int FromUserAddress, char *ToKernelAddress) {
    //Machine::ReadMem(int addr, int size, int *value)
    DEBUG('x', "CopyStringToKernel : (FromUserAddress 0x%x) (ToKernelAddress 0x%x)\n", FromUserAddress, ToKernelAddress);
    int c  /*, physAddr*/ ;
    do {
            //currentThread->space->Translate(FromUserAddress++,&physAddr, 1); //translation
            machine->ReadMem(FromUserAddress++, 1, &c);   //translation inside ReadMem
            DEBUG('x', "%c\n", (char) c);
            *ToKernelAddress++ = (char) c ;
            //DEBUG('x', "ToKernelAddress (%c)\n", *(ToKernelAddress-1));
    } while (*(ToKernelAddress-1) != NULL);
}
