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
void CopyToKernel(int FromUserAddress, int NumBytes, char *ToKernelAddress);
void CopyStringToKernel(int FromUserAddress, char *ToKernelAddress);
void Create_SystemCall();
void Open_SystemCall();
void Read_SystemCall();
void Write_SystemCall();
void Close_SystemCall();

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
    currentThread->RestoreUserState();
    machine->Run();
    
}

void threadExecFunc(int value){
    Thread * thread = (Thread *) value; 
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState(); // load page table register
    machine->Run();
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int vaddr = machine->ReadRegister(4);
    SpaceId pid = currentThread->space->getPCB()->getPID();
    if ((which == SyscallException) && (type == SC_Halt)) {
        printf("System Call: [%d] invoked Halt\n", pid);
	     DEBUG('x', "Shutdown, initiated by user program.\n");
   	  interrupt->Halt(); 
    }else if((which == SyscallException) && (type == SC_Exit)){
        printf("System Call: [%d] invoked Exit\n", pid);
        Exit_SystemCall(vaddr, pid);
        updatePC();
    }else if((which == SyscallException) && (type == SC_Exec)){
        printf("System Call: [%d] invoked Exec\n", pid);
	     Exec_SystemCall();
    }else if((which == SyscallException) && (type == SC_Join)){
        printf("System Call: [%d] invoked Join\n", pid);
        Join_SystemCall(vaddr);
    }else if((which == SyscallException) && (type == SC_Create)){
        printf("System Call: [%d] invoked Create\n", pid);
        Create_SystemCall();
    }else if((which == SyscallException) && (type == SC_Open)){
        printf("System Call: [%d] invoked Open\n", pid);
        Open_SystemCall();
    }else if((which == SyscallException) && (type == SC_Read)){
        printf("System Call: [%d] invoked Read\n", pid);
        Read_SystemCall();
    }else if((which == SyscallException) && (type == SC_Write)){
        printf("System Call: [%d] invoked Write\n", pid);
        Write_SystemCall();
    }else if((which == SyscallException) && (type == SC_Close)){
        printf("System Call: [%d] invoked Close\n", pid);
        Close_SystemCall();
    }else if((which == SyscallException) && (type == SC_Fork)){
        printf("System Call: [%d] invoked Fork\n", pid);
        Fork_SystemCall(pid);
    }else if((which == SyscallException) && (type == SC_Yield)){
        printf("System Call: [%d] invoked Yield\n", pid);
        machine->WriteRegister(PCReg, (machine->ReadRegister(PCReg) + 4) );
        currentThread->Yield();
        
    }else if((which == SyscallException) && (type == SC_Kill)){
        printf("System Call: [%d] invoked Kill\n", pid);
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
    int add = machine->ReadRegister(4);
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    //int add = machine->ReadRegister(PCReg);
    //add += 4;
    AddrSpace *space = currentThread->space->Clone();
    //currentThread->space = space;
    //space->RestoreState();      // load page table register
       DEBUG('x',
          "Process [%d] Fork: start at address [%d] with [%d] pages memory\n",
            currentPid,
            add,
            space->numPages);
 
    int oldadd = machine->ReadRegister(PCReg);
    machine->WriteRegister(PCReg,add);
    machine->WriteRegister(NextPCReg,add + 4);
    Thread *t = new Thread("forked thread");
    t->space = space;
    t->SaveUserState();
    pid = procTable->addProcess(t);

    t->space->getPCB()->setPID(pid);
    t->space->getPCB()->setParentPID(currentPid);
    DEBUG('x', "PID (%d) assigned for new thread\n", pid);
    machine->WriteRegister(2, pid);
    t->space->getPCB()->setParent(currentThread);
    t->space->getPCB()->setThreadPointer(t);
    //DEBUG('t', "Just before thread execution starts\n");
    (*procNumber)++;
    t->Fork(threadFunc, (int)t);

    machine->WriteRegister(PCReg,oldadd + 4);
    machine->WriteRegister(NextPCReg,oldadd + 8);
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
    	printf("Error, unable to open file: [%s]\n", filename);   ///ABORT program?
    	CleanupAndExit();
	machine->WriteRegister(2, -1);
    	return;
    }
    DEBUG('x', "Exec Program: %d loading %s\n", currentPid,filename);

    int oldadd = machine->ReadRegister(PCReg);

    space = new AddrSpace(executable); 
    thread = new Thread("child thread");
    thread->space = space;
    
    pid = procTable->addProcess(thread);
    thread->space->getPCB()->setPID(pid);
    machine->WriteRegister(2, pid);
    
    thread->space->getPCB()->setParent(currentThread);
    
    delete executable;			// close file
    (*procNumber)++;

    thread->Fork(threadExecFunc, (int) thread);

    machine->WriteRegister(PCReg,oldadd + 4);
    machine->WriteRegister(NextPCReg,oldadd + 8); 
    delete filename;
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
    if(procTable->returnProcess(currentThread->space->getPCB()->getParentPID()) != NULL ){
       if(currentThread->space->getPCB()->getParent() == currentThread->space->getPCB()->getJoinedID() ){
         procTable->returnProcess( currentThread->space->getPCB()->getParentPID() )->returnExit = status;
         currentThread->space->getPCB()->parentBlockSem->V();    
      }
    }else{
      printf("My parent is dead (Exit)\n");   
    }
    procTable->removeProcess(pid);
    printf("Process %d exits with status %d\n", pid, status);
    if (--(*procNumber))  
        currentThread->Finish();
    else
        interrupt->Halt();   //no other processes left 
}

void Kill_SystemCall(SpaceId killerPid, SpaceId pidKilled) {
   printf("System Call: %d invoked Kill\n", killerPid);
   ProcessControlBlock *pcb = procTable->returnProcess(pidKilled);
   Thread *t = pcb->getThreadPointer();
   if(pcb != NULL){
      t->killFlag = true;
      
      procTable->removeProcess(pidKilled);
      procNumber = procNumber - 1;
      printf("Process %d killed process %d\n", killerPid, pidKilled);
      machine->WriteRegister(2, -1);
   } else { 
      printf("Process %d cannot kill process %d: doesn't exist\n", killerPid, pidKilled);
      machine->WriteRegister(2, 0);
   }
}

void CleanupAndExit() {
    SpaceId pid;
    ProcessControlBlock *pcb = currentThread->space->getPCB();
    pid = pcb->getPID();
    
    DEBUG('x', "Cleaning up file table for process %s\n", 
            currentThread->getName());
    
    currentThread->space->getPCB()->cleanupFileEntries();
    
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
    int c  /*, physAddr*/ ;
    do {
            machine->ReadMem(FromUserAddress++, 1, &c);   //translation inside ReadMem
            *ToKernelAddress++ = (char) c ;
    } while (*(ToKernelAddress-1) != NULL);
}

void Open_SystemCall() {
    //OpenFileId Open(char *name);
    int fd, param;
    char filename[30];
    OpenFile * of;
    
    fd = -1; 
    param = machine->ReadRegister(4); 

    CopyStringToKernel(param,  filename);
    if ( (of = fileSystem->Open(filename)) != NULL)
        fd = currentThread->space->getPCB()->insertFile(of);  //add file entry to file table
    machine->WriteRegister(2,fd);
    increaseProgramCounter();
}

void CopyToKernel(int FromUserAddress, int NumBytes, char *ToKernelAddress) {
    int i, c /*, physAddr*/;
    for (i=0; i<NumBytes; i++){
        machine->ReadMem(FromUserAddress++, 1, &c); //translation inside ReadMem
        *ToKernelAddress++ = (char) c ;
    }
}



void CopyToUser(char *FromKernelAddress, int NumBytes, int ToUserAddress) {
    int i /*, physAddr*/;
    for (i=0; i<NumBytes; i++){
        machine->WriteMem(ToUserAddress, 1, (int)*FromKernelAddress ); //translation inside WriteMem
        FromKernelAddress++;
        ToUserAddress++;
    }
}





void Read_SystemCall() {
    //int Read(char *buffer, int size, OpenFileId id);
    int bufferVrtAddr, size, fd, numBytes;
    char * buffer;
    OpenFile * of;
    
    bufferVrtAddr = machine->ReadRegister(4); // 1st parameter
    size = machine->ReadRegister(5);  // 2nd parameter
    fd = machine->ReadRegister(6);    // 3rd parameter
    
    DEBUG('t', "Read System call : bufferVrtAddr(0x%x) size(%d) fd(%d)\n", bufferVrtAddr, size, fd );

    buffer = new char[size];  
    if (fd == ConsoleInput) {  //stdin
        for (int i=0;i<size;i++) buffer[i] = synchCons->GetChar();
        CopyToUser(buffer, size, bufferVrtAddr);
    }
    else if (fd == ConsoleOutput) { //stdout
        printf("Reading from stdout is not possible. Killing Program\n");
        CleanupAndExit();
    }
    else { //read from file system 
        if  ( (of = currentThread->space->getPCB()->returnFile(fd)) != NULL  ) {
            //DEBUG('x', "Reading File with openfile %p\n", of);        
            numBytes = of->Read(buffer,size);
            DEBUG('c', "Number of bytes read: %d\n", numBytes);
            CopyToUser(buffer, numBytes, bufferVrtAddr);
            // number of bytes read is returned
            machine->WriteRegister(2, numBytes);  
        }
        else {
            DEBUG('x', "Entry (%p)\n", of);
            DEBUG('x',"Error while reading file with fd (%d). Killing Program\n", fd);
            CleanupAndExit();
        }
    }
    increaseProgramCounter();
    delete buffer;
}


void Close_SystemCall() {
    //void Close(OpenFileId id);
    int fd;
    
    DEBUG('t', "Close System call\n");
    fd = machine->ReadRegister(4); // 1st parameter
    
    if (!currentThread->space->getPCB()->removeFile(fd)) {
        DEBUG('t',"Filesystem returned unsuccesfully when trying to close file with fd (%d). Killing Program\n",fd);
        CleanupAndExit();
    }
    
    increaseProgramCounter();
}


void Write_SystemCall() {
    //void Write(char *buffer, int size, OpenFileId id);
    int bufferVrtAddr, size, fd;
    char * buffer;
    OpenFile * of;
    
    bufferVrtAddr = machine->ReadRegister(4); // 1st parameter
    size = machine->ReadRegister(5);    // 2nd parameter
    fd = machine->ReadRegister(6);      // 3rd parameter
    DEBUG('t', "Write System call : bufferVrtAddr(0x%x) size(%d) fd(%d)\n", bufferVrtAddr, size, fd );

    buffer = new char[size];
    CopyToKernel(bufferVrtAddr, size, buffer);
    if (fd == ConsoleInput) {  //stdin
        printf("Writing to stdin is not possible. Killing Program\n");
        CleanupAndExit();
    }
    else if (fd == ConsoleOutput) { //stdout
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
        for (int i=0;i<size;i++) synchCons->PutChar(buffer[i]);
	synchCons->PutChar('\n');
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
    }
    else { //write to file system
        if  ( (of = currentThread->space->getPCB()->returnFile(fd)) != NULL  ) { //retrieve openfile pointer from file table
            of->Write(buffer,size);
        }
        else {
            DEBUG('t',"Error while writing  to file with fd (%d). Killing Program\n", fd);
            CleanupAndExit();
        }
    }
    increaseProgramCounter();
    delete buffer;
}


void Create_SystemCall() {
    int nameVrtAddr;
    char * filename;
    
    nameVrtAddr = machine->ReadRegister(4); // 1st parameter
    filename = new char[30];
    CopyStringToKernel(nameVrtAddr,  filename);
    if  (!fileSystem->Create(filename, 0)) {
        DEBUG('x',"Filesystem returned unsuccesfully when trying to create file %s. Killing Program\n",filename);
        CleanupAndExit();
    }
    increaseProgramCounter();
    delete filename;
}


