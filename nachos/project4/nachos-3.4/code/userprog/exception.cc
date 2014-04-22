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
#include <stdio.h>
void Join_SystemCall(int pid);
void Print_SystemCall();
//void Print_Syscall(int vAddress, int size, int values12, int values34);
void Exit_SystemCall(int status, SpaceId pid); 
void Kill_SystemCall(SpaceId killerPid, SpaceId pidKilled); 
void Fork_SystemCall(int pid);
void Exec_SystemCall();
void CleanupAndExit(); 
void increaseProgramCounter();   //instruction program counter is increased
void CopyStringToKernel(int FromUserAddress, char *ToKernelAddress);
//new for project 4
int handleMemoryFull(int pid);
int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;            // The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
      {
               result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
      }

      buf[n++] = *paddr;

      if ( !result ) {
    //translation failed
    return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

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
    if (which == SyscallException){
        switch(type){
            case SC_Halt:
                DEBUG('a', "Shutdown, initiated by user program.\n");
                interrupt->Halt();
                break;
            case SC_Exit:
                Exit_SystemCall(vaddr, pid);
                updatePC();
                break;
            case SC_Exec: 
                 printf("System Call: [%d] invoked Exec.\n",pid);
                 Exec_SystemCall();
                 break; 
            case SC_Join:
                 printf("System Call: [%d] invoked Join.\n",pid);
                //printf("System Call: [%d] invoked Join(%d)\n",pid,vaddr);
                Join_SystemCall(vaddr);
                break;
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
            case SC_Fork:
                printf("System Call: [%d] invoked Fork\n",pid);
                Fork_SystemCall(pid);
                break;
            case SC_Yield:
                printf("System Call: [%d] invoked Yield\n",pid);
                //printf("System Call: %d invoked Yield\n", SpaceId);
                updatePC();
                currentThread->SaveUserState(); 
                currentThread->space->SaveState();
                currentThread->Yield();
                break;
            case SC_Kill:
                printf("System Call: [%d] invoked Kill\n",pid);
                Kill_SystemCall(pid, vaddr);
                updatePC();
                break; 
            case SC_Print:
                Print_SystemCall();
                //Print_Syscall(machine->ReadRegister(4),
                //      machine->ReadRegister(5),
                //      machine->ReadRegister(6),
                //      machine->ReadRegister(7));
                break;
        
	        default:
                printf("Unexpected user mode exception %d %d\n", which, type);
                ASSERT(FALSE);
        }
    }else if( which == PageFaultException ){
        int vpn = machine->ReadRegister(BadVAddrReg)/PageSize; 
        TranslationEntry *pe = &currentThread->space->pageTable[vpn];
        int frame = bitmap->Find();
        pe->physicalPage = frame;
        if(frame == -1){
            //if no spaces are avaiable, reomve a page
            pe->physicalPage = handleMemoryFull(pid);
            
            //printf("No memory!");
            //interrupt->Halt();
        } 
        fifo[fifoSize] = pe->physicalPage;//add this page to the end of the queue
        fifoSize++;
        ASSERT(fifoSize<=NumPhysPages);        
        if(pe->location == EXECUTABLE) {
            int ret = 0;
            //printf("Read From EXECUTABLE offset:0x%x vpn[%d]->frame[%d]\n",
            //        pe->offset,vpn,pe->physicalPage);
            ret = currentThread->space->spaceExecutable->ReadAt(
                  (char*)&(machine->mainMemory[pe->physicalPage * PageSize]), 
                  PageSize, 
                  pe->offset);
            //printf("ReadAt ret=%d\n",ret);
            ASSERT(ret >0);
            char *c = &(machine->mainMemory[pe->physicalPage * PageSize]);
            for(int k=0; k<PageSize; k++){
                if(k==0){
                    DEBUG('b',"[0x%x][0x%x].",vpn*PageSize,c-machine->mainMemory);
                }else if ((*c)=='I'){
                    DEBUG('b',"%c[0x%x][0x%x]",*c,vpn*PageSize+k,c-machine->mainMemory);
                }else if(*c>='a' && *c<='z' || *c>='A'&&*c<='Z'){
                    DEBUG('b',"%c",*c);
                }else{
                    DEBUG('b',".");
                }
                c++;
            }
            DEBUG('b',"\n");
        }
        else if(pe->location == SWAPFILE) {
            int ret;
            //printf("Read From SWAPFILE[%d]\n",pe->swap);
            ret = swapFile->ReadAt(
                         &(machine->mainMemory[pe->physicalPage * PageSize]),
                         PageSize,
                         pe->swap * PageSize);
            ASSERT(ret == PageSize);
        }  
        else if(pe->location == NOT_ON_DISK){
            int a = 1;        //do nothing,just use this frame
        }
        pe->valid = TRUE;
        printf("L [%d]: [%d] -> [%d]\n",pid,pe->virtualPage,pe->physicalPage);
        //update the ipt entry
        ipt[pe->physicalPage].physicalPage = pe->physicalPage;
        ipt[pe->physicalPage].virtualPage = pe->virtualPage;
        ipt[pe->physicalPage].valid = pe->valid;
        ipt[pe->physicalPage].use = pe->use;
        ipt[pe->physicalPage].dirty = pe->dirty;
        ipt[pe->physicalPage].readOnly = pe->readOnly;
        ipt[pe->physicalPage].id = currentThread->space;
        stats->numPageFaults ++;
        machine->MemoryTieUpRate = bitmap->BitMapTieUpRate();
        DEBUG('t',"Memory Usage Rate=%.2f\%\n",100*machine->MemoryTieUpRate);
        
    }else if( which == ReadOnlyException ){
        printf("ReadOnlyException\n");
    }else if( which == BusErrorException ){
        printf("BusErrorException\n");
    }else if( which == AddressErrorException ){
        printf("AddressErrorException\n");
    }else if( which == OverflowException ){
        printf("OverflowException\n");
    }else if( which == IllegalInstrException ){
        printf("IllegalInstrException\n");
        interrupt->Halt();
    }else if( which == NumExceptionTypes ){
        printf("NumExceptionTypes\n");
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
    const int maxfilename = 512;
    char * filename;
    OpenFile * executable;
    AddrSpace * space;
    Thread * thread;
    SpaceId pid;
    SpaceId currentPid = currentThread->space->getPCB()->getPID();
     
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    add= machine->ReadRegister(4); // 1st parameter, file name
    //sizeadd = machine->ReadRegister(5); // the 2nd parameter, file size
    
    filename = new char[maxfilename];
    CopyStringToKernel(add,filename);
    
    printf("Exec Program: %d loading [\"%s\"]\n", currentPid,filename);
    executable = fileSystem->Open(filename);
    
    if (executable == NULL) {
    	printf("Error, unable to open file: [%s]\n", filename); 
    	CleanupAndExit();
        machine->WriteRegister(2, -1);
    	return;
    }


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
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Print_SystemCall() {
    DEBUG('t',"SC_Print, initiate by user program.\n");
    // Get the string from user space.
    int vaddr = machine->ReadRegister(4);
    //vaddr = 0x191;
    int size = 0;
    bool ret;
    int *paddr = new int;
    buf[BUF_SIZE - 1] = '\0';               // For safety.
    do{
        do{
        // Invoke ReadMem to read the contents from user space
        ret = machine->ReadMem(vaddr,    // Location to be read
            1,      // Size of data to be read
            (int*)&(buf[size])   // where the read contents 
            );
        }while(ret==FALSE);// are stored
        // Compute next address
        //printf("debug:vaddr:[0x%x][%d]\n",vaddr,buf[size]);
        vaddr+=sizeof(char);
        size++;
    } while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

    size--;
    DEBUG('t', "Size of string = %d\n", size);

    printf("%s", buf);
    bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
    updatePC();

}
/*
void Print_Syscall(int vAddress, int size, int values12, int values34)
{

    int input = 1;
    char* message = new char[size];
    if(copyin(vAddress, size, message) == -1)
    {
        printf("BAD ADDRESS\n");
    }
    for(int i=0; i<size; i++)
    {


        //add integers to the output
        if(message[i] != NULL)
        {
            if(message[i] == '%' && (message[i+1] == 'd' || message[i+1] == 'i'))
            {

                switch (input)
                {
                case 1:
                    printf("%i",values12/100);
                    break;
                case 2:
                    printf("%i",values12%100);
                    break;
                case 3:
                    printf("%i",values34/100);
                    break;
                case 4:
                    printf("%i",values34%100);
                    break;
                }
                input++;
                i++;
            }
            else if(message[i] == '\\' && (message[i+1] == '\"')) //Need to print quotes in some outputs
            {

                printf("\"");
                i++;
            }
            else if(message[i] == '\\' && (message[i+1] == 'n')) //Need to print new lines in some outputs
            {

                printf("\n");
                i++;
            }
            else if(message[i] == '\\' && (message[i+1] == 't')) //Need to print new lines in some outputs
            {

                printf("\t");
                i++;
            }
            else
            {
                //print the next char

                printf("%c", message[i]);
            }
        }
    }
    printf("\n");
}
*/
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
    for(int i=0; i<currentThread->space->numPages; i++){
        TranslationEntry *pe = &(currentThread->space->pageTable[i]);
        if(pe->location == SWAPFILE){
            swapBitMap->Clear(pe->swap);
        }
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
    DEBUG('x', "CopyStringToKernel : (FromUserAddress 0x%x) (ToKernelAddress 0x%x)\n", FromUserAddress, ToKernelAddress);
    int c  /*, physAddr*/ ;
    bool ret;
    do {
            //currentThread->space->Translate(FromUserAddress++,&physAddr, 1); //translation
        do{
        // Invoke ReadMem to read the contents from user space
        ret = machine->ReadMem(FromUserAddress,    // Location to be read
            1,      // Size of data to be read
            &c);   // where the read contents
        }while(ret==FALSE);// are stored            
            DEBUG('x', "%c\n", (char) c);
            *ToKernelAddress++ = (char) c ;
            FromUserAddress++;
            //DEBUG('x', "ToKernelAddress (%c)\n", *(ToKernelAddress-1));
    } while (*(ToKernelAddress-1) != NULL);
}

int handleMemoryFull(int pid)
{
    int removePage;
    int ret = 0;
    removePage = fifo[0];//remove the first page in the queue
    for(int i = 1; i < fifoSize; i++) {
        fifo[i-1] = fifo[i];
    }
    fifoSize--;
    TranslationEntry *pe = &(ipt[removePage].id->pageTable[ipt[removePage].virtualPage]);
            
    if(pe->dirty == TRUE)//check if the page is dirty
    {
        //if the page is already in the swapfile, overwrite it
        if(pe->location == SWAPFILE){
/*            printf("already in swapfile,swap frame[%d] vpn[%d] to swap file block[%d]\n",
                    removePage,
                    ipt[removePage].virtualPage,
                    pe->swap
                  );
*/
            ret = swapFile->WriteAt(
                    &(machine->mainMemory[removePage*PageSize]),
                    PageSize,
                    pe->swap * PageSize
                   );
            ASSERT(ret==PageSize);
        }else {
            int swapIndex = swapBitMap->Find();//find an available index in the swapfile
//            printf("swap frame[%d] vpn[%d] to swap file block[%d]\n",removePage,ipt[removePage].virtualPage,swapIndex);
            if(swapIndex == -1) {
                printf("%s","Swap File is not big enough\n");
            }
            ret = swapFile->WriteAt(
                    &(machine->mainMemory[removePage*PageSize]),
                    PageSize,
                    swapIndex*PageSize
                   );//write the page to the swapfile
            ASSERT(ret==PageSize);
            pe->location = SWAPFILE;//update location field in pagetable
            pe->swap = swapIndex;//update swap field in pagetable
        }
        printf("S [%d]: [%d]\n",pid,removePage);
    }else{
        printf("E [%d]: [%d]\n",pid,removePage);
    }
    pe->valid = FALSE;

    return removePage;


}

